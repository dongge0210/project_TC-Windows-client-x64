using System;
using System.Collections.Generic;

namespace WPF_UI1.GPU
{
    /// <summary>
    /// GPU信息接口
    /// </summary>
    public interface IGPUInfo
    {
        /// <summary>
        /// GPU设备ID
        /// </summary>
        string DeviceId { get; }

        /// <summary>
        /// GPU名称
        /// </summary>
        string Name { get; }

        /// <summary>
        /// GPU厂商
        /// </summary>
        GPUVendor Vendor { get; }

        /// <summary>
        /// GPU类型
        /// </summary>
        GPUType Type { get; }

        /// <summary>
        /// 显存大小（字节）
        /// </summary>
        long MemorySize { get; }

        /// <summary>
        /// 核心时钟频率（MHz）
        /// </summary>
        float CoreClock { get; }

        /// <summary>
        /// 显存时钟频率（MHz）
        /// </summary>
        float MemoryClock { get; }

        /// <summary>
        /// 驱动程序版本
        /// </summary>
        string DriverVersion { get; }

        /// <summary>
        /// 设备位置
        /// </summary>
        string DeviceLocation { get; }

        /// <summary>
        /// 是否为主GPU
        /// </summary>
        bool IsPrimary { get; }
    }

    /// <summary>
    /// GPU监控数据接口
    /// </summary>
    public interface IGPUMonitorData
    {
        /// <summary>
        /// GPU温度（摄氏度）
        /// </summary>
        float Temperature { get; }

        /// <summary>
        /// GPU使用率（百分比）
        /// </summary>
        float Usage { get; }

        /// <summary>
        /// 显存使用量（字节）
        /// </summary>
        long MemoryUsed { get; }

        /// <summary>
        /// 显存使用率（百分比）
        /// </summary>
        float MemoryUsagePercent { get; }

        /// <summary>
        /// 当前核心时钟频率（MHz）
        /// </summary>
        float CurrentCoreClock { get; }

        /// <summary>
        /// 当前显存时钟频率（MHz）
        /// </summary>
        float CurrentMemoryClock { get; }

        /// <summary>
        /// 功耗（瓦特）
        /// </summary>
        float PowerDraw { get; }

        /// <summary>
        /// 风扇转速（RPM）
        /// </summary>
        float FanSpeed { get; }

        /// <summary>
        /// 风扇使用率（百分比）
        /// </summary>
        float FanSpeedPercent { get; }

        /// <summary>
        /// 数据采集时间
        /// </summary>
        DateTime Timestamp { get; }
    }

    /// <summary>
    /// GPU提供者接口
    /// </summary>
    public interface IGPUProvider
    {
        /// <summary>
        /// 提供者名称
        /// </summary>
        string Name { get; }

        /// <summary>
        /// 支持的GPU厂商
        /// </summary>
        GPUVendor[] SupportedVendors { get; }

        /// <summary>
        /// 是否可用
        /// </summary>
        bool IsAvailable { get; }

        /// <summary>
        /// 初始化提供者
        /// </summary>
        /// <returns>是否成功</returns>
        bool Initialize();

        /// <summary>
        /// 获取GPU列表
        /// </summary>
        /// <returns>GPU信息列表</returns>
        List<IGPUInfo> GetGPUs();

        /// <summary>
        /// 获取GPU监控数据
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <returns>监控数据</returns>
        IGPUMonitorData GetMonitorData(string deviceId);

        /// <summary>
        /// 开始监控
        /// </summary>
        void StartMonitoring();

        /// <summary>
        /// 停止监控
        /// </summary>
        void StopMonitoring();

        /// <summary>
        /// 释放资源
        /// </summary>
        void Dispose();

        /// <summary>
        /// 监控数据更新事件
        /// </summary>
        event EventHandler<GPUMonitorDataEventArgs> MonitorDataUpdated;
    }

    /// <summary>
    /// GPU厂商枚举
    /// </summary>
    public enum GPUVendor
    {
        Unknown,
        NVIDIA,
        AMD,
        Intel,
        ARM,
        Qualcomm,
        Other
    }

    /// <summary>
    /// GPU类型枚举
    /// </summary>
    public enum GPUType
    {
        Unknown,
        Discrete,      // 独立显卡
        Integrated,    // 集成显卡
        Virtual,       // 虚拟显卡
        External       // 外置显卡
    }

    /// <summary>
    /// GPU监控数据事件参数
    /// </summary>
    public class GPUMonitorDataEventArgs : EventArgs
    {
        public string DeviceId { get; set; }
        public IGPUMonitorData Data { get; set; }
    }

    /// <summary>
    /// GPU性能计数器类型
    /// </summary>
    public enum GPUPerformanceCounter
    {
        Usage,
        Temperature,
        MemoryUsage,
        CoreClock,
        MemoryClock,
        PowerDraw,
        FanSpeed
    }

    /// <summary>
    /// GPU超频接口
    /// </summary>
    public interface IGPUOverclock
    {
        /// <summary>
        /// 是否支持超频
        /// </summary>
        bool SupportsOverclocking { get; }

        /// <summary>
        /// 设置核心时钟偏移
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <param name="offset">偏移量（MHz）</param>
        /// <returns>是否成功</returns>
        bool SetCoreClockOffset(string deviceId, int offset);

        /// <summary>
        /// 设置显存时钟偏移
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <param name="offset">偏移量（MHz）</param>
        /// <returns>是否成功</returns>
        bool SetMemoryClockOffset(string deviceId, int offset);

        /// <summary>
        /// 设置功耗限制
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <param name="powerLimit">功耗限制（百分比）</param>
        /// <returns>是否成功</returns>
        bool SetPowerLimit(string deviceId, int powerLimit);

        /// <summary>
        /// 设置风扇曲线
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <param name="fanCurve">风扇曲线数据</param>
        /// <returns>是否成功</returns>
        bool SetFanCurve(string deviceId, Dictionary<int, int> fanCurve);

        /// <summary>
        /// 重置为默认设置
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <returns>是否成功</returns>
        bool ResetToDefault(string deviceId);
    }

    /// <summary>
    /// GPU配置文件接口
    /// </summary>
    public interface IGPUProfile
    {
        /// <summary>
        /// 配置文件名称
        /// </summary>
        string Name { get; set; }

        /// <summary>
        /// 核心时钟偏移
        /// </summary>
        int CoreClockOffset { get; set; }

        /// <summary>
        /// 显存时钟偏移
        /// </summary>
        int MemoryClockOffset { get; set; }

        /// <summary>
        /// 功耗限制
        /// </summary>
        int PowerLimit { get; set; }

        /// <summary>
        /// 风扇曲线
        /// </summary>
        Dictionary<int, int> FanCurve { get; set; }

        /// <summary>
        /// 应用配置文件
        /// </summary>
        /// <param name="deviceId">设备ID</param>
        /// <param name="overclockProvider">超频提供者</param>
        /// <returns>是否成功</returns>
        bool Apply(string deviceId, IGPUOverclock overclockProvider);
    }
}