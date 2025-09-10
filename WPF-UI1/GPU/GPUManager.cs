using System;
using System.Collections.Generic;
using System.Linq;
using Serilog;

namespace WPF_UI1.GPU
{
    /// <summary>
    /// GPU管理器
    /// </summary>
    public class GPUManager : IDisposable
    {
        private readonly List<IGPUProvider> _providers;
        private readonly List<IGPUInfo> _detectedGPUs;
        private readonly Dictionary<string, IGPUMonitorData> _cachedMonitorData;
        private bool _disposed = false;
        private bool _isMonitoring = false;

        public GPUManager()
        {
            _providers = new List<IGPUProvider>();
            _detectedGPUs = new List<IGPUInfo>();
            _cachedMonitorData = new Dictionary<string, IGPUMonitorData>();
            
            InitializeProviders();
        }

        /// <summary>
        /// 已检测到的GPU列表
        /// </summary>
        public IReadOnlyList<IGPUInfo> DetectedGPUs => _detectedGPUs.AsReadOnly();

        /// <summary>
        /// 已注册的提供者列表
        /// </summary>
        public IReadOnlyList<IGPUProvider> Providers => _providers.AsReadOnly();

        /// <summary>
        /// 是否正在监控
        /// </summary>
        public bool IsMonitoring => _isMonitoring;

        /// <summary>
        /// GPU监控数据更新事件
        /// </summary>
        public event EventHandler<GPUMonitorDataEventArgs> MonitorDataUpdated;

        /// <summary>
        /// 初始化GPU提供者
        /// </summary>
        private void InitializeProviders()
        {
            try
            {
                // 注册内置提供者
                RegisterProvider(new WindowsGPUProvider());
                RegisterProvider(new LibreHardwareGPUProvider());
                
                Log.Information("GPU管理器已初始化，注册了 {Count} 个提供者", _providers.Count);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化GPU提供者失败");
            }
        }

        /// <summary>
        /// 注册GPU提供者
        /// </summary>
        /// <param name="provider">GPU提供者</param>
        public void RegisterProvider(IGPUProvider provider)
        {
            if (provider == null) return;

            try
            {
                if (provider.Initialize())
                {
                    _providers.Add(provider);
                    provider.MonitorDataUpdated += OnProviderMonitorDataUpdated;
                    Log.Information("GPU提供者已注册: {ProviderName}", provider.Name);
                }
                else
                {
                    Log.Warning("GPU提供者初始化失败: {ProviderName}", provider.Name);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "注册GPU提供者失败: {ProviderName}", provider.Name);
            }
        }

        /// <summary>
        /// 扫描GPU设备
        /// </summary>
        public void ScanGPUs()
        {
            try
            {
                _detectedGPUs.Clear();

                foreach (var provider in _providers.Where(p => p.IsAvailable))
                {
                    try
                    {
                        var gpus = provider.GetGPUs();
                        if (gpus != null)
                        {
                            _detectedGPUs.AddRange(gpus);
                            Log.Information("提供者 {ProviderName} 检测到 {Count} 个GPU", provider.Name, gpus.Count);
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Error(ex, "提供者 {ProviderName} 扫描GPU失败", provider.Name);
                    }
                }

                // 去除重复的GPU（根据DeviceId）
                var uniqueGPUs = _detectedGPUs
                    .GroupBy(gpu => gpu.DeviceId)
                    .Select(g => g.First())
                    .ToList();

                _detectedGPUs.Clear();
                _detectedGPUs.AddRange(uniqueGPUs);

                Log.Information("GPU扫描完成，共检测到 {Count} 个GPU", _detectedGPUs.Count);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "扫描GPU设备失败");
            }
        }

        /// <summary>
        /// 获取GPU监控数据
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <returns>监控数据</returns>
        public IGPUMonitorData GetMonitorData(string deviceId)
        {
            try
            {
                // 先从缓存获取
                if (_cachedMonitorData.TryGetValue(deviceId, out var cachedData))
                {
                    // 检查数据是否过期（超过5秒）
                    if (DateTime.Now.Subtract(cachedData.Timestamp).TotalSeconds < 5)
                    {
                        return cachedData;
                    }
                }

                // 从提供者获取新数据
                foreach (var provider in _providers.Where(p => p.IsAvailable))
                {
                    try
                    {
                        var data = provider.GetMonitorData(deviceId);
                        if (data != null)
                        {
                            _cachedMonitorData[deviceId] = data;
                            return data;
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Debug(ex, "提供者 {ProviderName} 获取监控数据失败: {DeviceId}", provider.Name, deviceId);
                    }
                }

                return null;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "获取GPU监控数据失败: {DeviceId}", deviceId);
                return null;
            }
        }

        /// <summary>
        /// 开始监控所有GPU
        /// </summary>
        public void StartMonitoring()
        {
            if (_isMonitoring) return;

            try
            {
                foreach (var provider in _providers.Where(p => p.IsAvailable))
                {
                    provider.StartMonitoring();
                }

                _isMonitoring = true;
                Log.Information("GPU监控已开始");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "开始GPU监控失败");
            }
        }

        /// <summary>
        /// 停止监控
        /// </summary>
        public void StopMonitoring()
        {
            if (!_isMonitoring) return;

            try
            {
                foreach (var provider in _providers)
                {
                    provider.StopMonitoring();
                }

                _isMonitoring = false;
                Log.Information("GPU监控已停止");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "停止GPU监控失败");
            }
        }

        /// <summary>
        /// 根据厂商获取GPU
        /// </summary>
        /// <param name="vendor">GPU厂商</param>
        /// <returns>GPU列表</returns>
        public List<IGPUInfo> GetGPUsByVendor(GPUVendor vendor)
        {
            return _detectedGPUs.Where(gpu => gpu.Vendor == vendor).ToList();
        }

        /// <summary>
        /// 根据类型获取GPU
        /// </summary>
        /// <param name="type">GPU类型</param>
        /// <returns>GPU列表</returns>
        public List<IGPUInfo> GetGPUsByType(GPUType type)
        {
            return _detectedGPUs.Where(gpu => gpu.Type == type).ToList();
        }

        /// <summary>
        /// 获取主GPU
        /// </summary>
        /// <returns>主GPU信息</returns>
        public IGPUInfo GetPrimaryGPU()
        {
            return _detectedGPUs.FirstOrDefault(gpu => gpu.IsPrimary);
        }

        /// <summary>
        /// 获取支持指定厂商的提供者
        /// </summary>
        /// <param name="vendor">GPU厂商</param>
        /// <returns>提供者列表</returns>
        public List<IGPUProvider> GetProvidersForVendor(GPUVendor vendor)
        {
            return _providers.Where(p => p.SupportedVendors.Contains(vendor)).ToList();
        }

        /// <summary>
        /// 提供者监控数据更新事件处理
        /// </summary>
        /// <param name="sender">发送者</param>
        /// <param name="e">事件参数</param>
        private void OnProviderMonitorDataUpdated(object sender, GPUMonitorDataEventArgs e)
        {
            try
            {
                // 更新缓存
                _cachedMonitorData[e.DeviceId] = e.Data;
                
                // 转发事件
                MonitorDataUpdated?.Invoke(this, e);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "处理提供者监控数据更新事件失败");
            }
        }

        /// <summary>
        /// 释放资源
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                StopMonitoring();

                foreach (var provider in _providers)
                {
                    try
                    {
                        provider.MonitorDataUpdated -= OnProviderMonitorDataUpdated;
                        provider.Dispose();
                    }
                    catch (Exception ex)
                    {
                        Log.Error(ex, "释放GPU提供者资源失败: {ProviderName}", provider.Name);
                    }
                }

                _providers.Clear();
                _detectedGPUs.Clear();
                _cachedMonitorData.Clear();

                _disposed = true;
                Log.Information("GPU管理器已释放");
            }
        }
    }

    /// <summary>
    /// Windows WMI GPU提供者
    /// </summary>
    internal class WindowsGPUProvider : IGPUProvider
    {
        public string Name => "Windows WMI GPU Provider";
        public GPUVendor[] SupportedVendors => new[] { GPUVendor.NVIDIA, GPUVendor.AMD, GPUVendor.Intel };
        public bool IsAvailable => Environment.OSVersion.Platform == PlatformID.Win32NT;

        public event EventHandler<GPUMonitorDataEventArgs> MonitorDataUpdated;

        public bool Initialize()
        {
            // WMI提供者初始化逻辑
            return IsAvailable;
        }

        public List<IGPUInfo> GetGPUs()
        {
            // 实现Windows WMI GPU检测逻辑
            // 这里提供基础框架
            return new List<IGPUInfo>();
        }

        public IGPUMonitorData GetMonitorData(string deviceId)
        {
            // 实现WMI监控数据获取逻辑
            return null;
        }

        public void StartMonitoring() { /* 实现监控启动逻辑 */ }
        public void StopMonitoring() { /* 实现监控停止逻辑 */ }
        public void Dispose() { /* 实现资源释放逻辑 */ }
    }

    /// <summary>
    /// LibreHardwareMonitor GPU提供者
    /// </summary>
    internal class LibreHardwareGPUProvider : IGPUProvider
    {
        public string Name => "LibreHardwareMonitor GPU Provider";
        public GPUVendor[] SupportedVendors => new[] { GPUVendor.NVIDIA, GPUVendor.AMD, GPUVendor.Intel };
        public bool IsAvailable => true;

        public event EventHandler<GPUMonitorDataEventArgs> MonitorDataUpdated;

        public bool Initialize()
        {
            // LibreHardwareMonitor初始化逻辑
            return true;
        }

        public List<IGPUInfo> GetGPUs()
        {
            // 实现LibreHardwareMonitor GPU检测逻辑
            return new List<IGPUInfo>();
        }

        public IGPUMonitorData GetMonitorData(string deviceId)
        {
            // 实现LibreHardwareMonitor监控数据获取逻辑
            return null;
        }

        public void StartMonitoring() { /* 实现监控启动逻辑 */ }
        public void StopMonitoring() { /* 实现监控停止逻辑 */ }
        public void Dispose() { /* 实现资源释放逻辑 */ }
    }
}