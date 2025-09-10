using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using WPF_UI1.Models;
using WPF_UI1.Services;
using Serilog;
using System.Linq; // ����: ���� FirstOrDefault / SequenceEqual

namespace WPF_UI1.ViewModels
{
    public partial class MainWindowViewModel : ObservableObject
    {
        private readonly SharedMemoryService _sharedMemoryService;
        private readonly DispatcherTimer _updateTimer;
        private const int MAX_CHART_POINTS = 60;
        private int _consecutiveErrors = 0;
        private const int MAX_CONSECUTIVE_ERRORS = 5;

        [ObservableProperty]
        private bool isConnected;

        [ObservableProperty]
        private string connectionStatus = "��������...";

        [ObservableProperty]
        private string windowTitle = "ϵͳӲ��������";

        [ObservableProperty]
        private string lastUpdateTime = "��δ����";

        #region CPU Properties
        [ObservableProperty]
        private string cpuName = "���ڼ��...";

        [ObservableProperty]
        private int physicalCores;

        [ObservableProperty]
        private int logicalCores;

        [ObservableProperty]
        private int performanceCores;

        [ObservableProperty]
        private int efficiencyCores;

        [ObservableProperty]
        private double cpuUsage;

        [ObservableProperty]
        private bool hyperThreading;

        [ObservableProperty]
        private bool virtualization;

        [ObservableProperty]
        private double cpuTemperature;
        #endregion

        #region Memory Properties
        [ObservableProperty]
        private string totalMemory = "�����...";

        [ObservableProperty]
        private string usedMemory = "�����...";

        [ObservableProperty]
        private string availableMemory = "�����...";

        [ObservableProperty]
        private double memoryUsagePercent;
        #endregion

        #region GPU Properties
        [ObservableProperty]
        private ObservableCollection<GpuData> gpus = new();

        [ObservableProperty]
        private GpuData? selectedGpu;

        [ObservableProperty]
        private double gpuTemperature;
        #endregion

        #region Network Properties
        [ObservableProperty]
        private ObservableCollection<NetworkAdapterData> networkAdapters = new();

        [ObservableProperty]
        private NetworkAdapterData? selectedNetworkAdapter;
        #endregion

        #region Disk Properties
        [ObservableProperty]
        private ObservableCollection<DiskData> disks = new();

        [ObservableProperty]
        private DiskData? selectedDisk;

        // �������������̷���
        [ObservableProperty]
        private ObservableCollection<PhysicalDiskView> physicalDisks = new();

        [ObservableProperty]
        private PhysicalDiskView? selectedPhysicalDisk;
        #endregion

        #region TPM Properties
        [ObservableProperty]
        private bool hasTpm;

        [ObservableProperty]
        private string tpmManufacturer = "�����";

        [ObservableProperty]
        private string tpmManufacturerId = string.Empty;

        [ObservableProperty]
        private string tpmVersion = string.Empty;

        [ObservableProperty]
        private string tpmFirmwareVersion = string.Empty;

        [ObservableProperty]
        private string tpmStatus = string.Empty;

        [ObservableProperty]
        private bool tpmEnabled;

        [ObservableProperty]
        private bool tpmIsActivated;

        [ObservableProperty]
        private bool tpmIsOwned;

        [ObservableProperty]
        private bool tpmReady;

        [ObservableProperty]
        private bool tpmTbsAvailable;

        [ObservableProperty]
        private bool tpmPhysicalPresenceRequired;

        [ObservableProperty]
        private uint tpmSpecVersion;

        [ObservableProperty]
        private uint tpmTbsVersion;

        [ObservableProperty]
        private string tpmErrorMessage = string.Empty;
        
        [ObservableProperty]
        private string tpmDetectionMethod = string.Empty;
        
        [ObservableProperty]
        private bool tpmWmiDetectionWorked;
        
        [ObservableProperty]
        private bool tpmTbsDetectionWorked;
        #endregion

        #region Chart Properties
        public ObservableCollection<ISeries> CpuTemperatureSeries { get; } = new();
        public ObservableCollection<ISeries> GpuTemperatureSeries { get; } = new();

        private readonly ObservableCollection<double> _cpuTempData = new();
        private readonly ObservableCollection<double> _gpuTempData = new();
        #endregion

        public MainWindowViewModel(SharedMemoryService sharedMemoryService)
        {
            _sharedMemoryService = sharedMemoryService;
            
            InitializeCharts();
            
            // ���ø��¶�ʱ��
            _updateTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(500) // ��΢����Ƶ���Լ���CPUռ��
            };
            _updateTimer.Tick += UpdateTimer_Tick;
            _updateTimer.Start();

            // ���γ�������
            TryConnect();
        }

        private void InitializeCharts()
        {
            // CPU�¶�ͼ��
            CpuTemperatureSeries.Add(new LineSeries<double>
            {
                Values = _cpuTempData,
                Name = "CPU�¶�",
                Stroke = new SolidColorPaint(SKColors.DeepSkyBlue) { StrokeThickness = 2 },
                Fill = null,
                GeometrySize = 0 // �������ݵ�
            });

            // GPU�¶�ͼ��
            GpuTemperatureSeries.Add(new LineSeries<double>
            {
                Values = _gpuTempData,
                Name = "GPU�¶�",
                Stroke = new SolidColorPaint(SKColors.Orange) { StrokeThickness = 2 },
                Fill = null,
                GeometrySize = 0
            });
        }

        private async void UpdateTimer_Tick(object? sender, EventArgs e)
        {
            await UpdateSystemInfoAsync();
        }

        private async Task UpdateSystemInfoAsync()
        {
            try
            {
                var systemInfo = await Task.Run(() => _sharedMemoryService.ReadSystemInfo());
                
                if (systemInfo != null)
                {
                    _consecutiveErrors = 0; // ���ô��������

                    if (!IsConnected)
                    {
                        IsConnected = true;
                        ConnectionStatus = "������";
                        WindowTitle = "ϵͳӲ��������";
                    }

                    UpdateSystemData(systemInfo);
                    LastUpdateTime = DateTime.Now.ToString("HH:mm:ss");
                }
                else
                {
                    _consecutiveErrors++;
                    
                    if (_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
                    {
                        if (IsConnected)
                        {
                            IsConnected = false;
                            ConnectionStatus = "�����ѶϿ� - ������������...";
                            WindowTitle = "ϵͳӲ�������� - �����ж�";
                            
                            // ��ʾ����״̬������
                            ShowDisconnectedState();
                            
                            // ������������
                            await Task.Run(() => TryConnect());
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "����ϵͳ��Ϣʱ��������");
                _consecutiveErrors++;
                
                if (_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
                {
                    IsConnected = false;
                    ConnectionStatus = $"����: {ex.Message}";
                    ShowErrorState(ex.Message);
                }
            }
        }

        private void TryConnect()
        {
            Log.Information("�������ӹ����ڴ�...");
            
            if (_sharedMemoryService.Initialize())
            {
                IsConnected = true;
                ConnectionStatus = "������";
                WindowTitle = "ϵͳӲ��������";
                _consecutiveErrors = 0;
                Log.Information("�����ڴ����ӳɹ�");
            }
            else
            {
                IsConnected = false;
                ConnectionStatus = $"����ʧ��: {_sharedMemoryService.LastError}";
                WindowTitle = "ϵͳӲ�������� - δ����";
                Log.Warning($"�����ڴ�����ʧ��: {_sharedMemoryService.LastError}");
                
                ShowDisconnectedState();
            }
        }

        private void ShowDisconnectedState()
        {
            CpuName = "? �����ѶϿ�";
            TotalMemory = "δ����";
            UsedMemory = "δ����";
            AvailableMemory = "δ����";
            
            // ��ռ���
            Gpus.Clear();
            NetworkAdapters.Clear();
            Disks.Clear();
            PhysicalDisks.Clear();
            
            // ����ѡ��
            SelectedGpu = null;
            SelectedNetworkAdapter = null;
            SelectedDisk = null;
            SelectedPhysicalDisk = null;
        }

        private void ShowErrorState(string errorMessage)
        {
            CpuName = $"?? ���ݶ�ȡʧ��: {errorMessage}";
            TotalMemory = "��ȡʧ��";
            UsedMemory = "��ȡʧ��";
            AvailableMemory = "��ȡʧ��";
        }

        private void UpdateSystemData(SystemInfo systemInfo)
        {
            try
            {
                // ����CPU��Ϣ - ����������֤
                CpuName = ValidateAndSetString(systemInfo.CpuName, "δ֪������");
                PhysicalCores = ValidateAndSetInt(systemInfo.PhysicalCores, "��������");
                LogicalCores = ValidateAndSetInt(systemInfo.LogicalCores, "�߼�����");
                PerformanceCores = ValidateAndSetInt(systemInfo.PerformanceCores, "���ܺ���");
                EfficiencyCores = ValidateAndSetInt(systemInfo.EfficiencyCores, "��Ч����");
                CpuUsage = ValidateAndSetDouble(systemInfo.CpuUsage, "CPUʹ����");
                HyperThreading = systemInfo.HyperThreading;
                Virtualization = systemInfo.Virtualization;
                CpuTemperature = ValidateAndSetDouble(systemInfo.CpuTemperature, "CPU�¶�");

                // �����ڴ���Ϣ - ����������֤
                TotalMemory = systemInfo.TotalMemory > 0 ? FormatBytes(systemInfo.TotalMemory) : "δ��⵽";
                UsedMemory = systemInfo.UsedMemory > 0 ? FormatBytes(systemInfo.UsedMemory) : "δ֪";
                AvailableMemory = systemInfo.AvailableMemory > 0 ? FormatBytes(systemInfo.AvailableMemory) : "δ֪";
                
                MemoryUsagePercent = systemInfo.TotalMemory > 0 
                    ? (double)systemInfo.UsedMemory / systemInfo.TotalMemory * 100.0 
                    : 0.0;

                // ����GPU��Ϣ
                UpdateCollection(Gpus, systemInfo.Gpus ?? new List<GpuData>());
                if (SelectedGpu == null && Gpus.Count > 0)
                    SelectedGpu = Gpus[0];

                GpuTemperature = ValidateAndSetDouble(systemInfo.GpuTemperature, "GPU�¶�");

                // �ڸ�������������ǰ���浱ǰѡ���������ص���
                string? previousNetKey = SelectedNetworkAdapter != null
                    ? $"{SelectedNetworkAdapter.Name}|{SelectedNetworkAdapter.Mac}"
                    : null;

                // ����������Ϣ
                UpdateCollection(NetworkAdapters, systemInfo.Adapters ?? new List<NetworkAdapterData>());

                // ���Իָ�֮ǰ��ѡ��
                if (previousNetKey != null)
                {
                    var restored = NetworkAdapters.FirstOrDefault(a => $"{a.Name}|{a.Mac}" == previousNetKey);
                    if (restored != null)
                    {
                        SelectedNetworkAdapter = restored; // �ָ�ԭѡ��
                    }
                }

                if (SelectedNetworkAdapter == null && NetworkAdapters.Count > 0)
                    SelectedNetworkAdapter = NetworkAdapters[0];

                // �ڸ��´���ǰ���浱ǰѡ��
                string? previousDiskKey = SelectedDisk != null ? $"{SelectedDisk.Letter}:{SelectedDisk.Label}" : null;
                string? previousPhysicalKey = SelectedPhysicalDisk?.Disk?.SerialNumber;

                // ���´�����Ϣ
                UpdateCollection(Disks, systemInfo.Disks ?? new List<DiskData>());

                // ����/�����������̷���
                BuildOrUpdatePhysicalDisks(systemInfo);

                if (previousDiskKey != null)
                {
                    var restoredDisk = Disks.FirstOrDefault(d => $"{d.Letter}:{d.Label}" == previousDiskKey);
                    if (restoredDisk != null)
                        SelectedDisk = restoredDisk;
                }
                if (SelectedDisk == null && Disks.Count > 0)
                    SelectedDisk = Disks[0];

                if (previousPhysicalKey != null)
                {
                    var restoredPd = PhysicalDisks.FirstOrDefault(p => p.Disk.SerialNumber == previousPhysicalKey);
                    if (restoredPd != null)
                        SelectedPhysicalDisk = restoredPd;
                }
                if (SelectedPhysicalDisk == null && PhysicalDisks.Count > 0)
                    SelectedPhysicalDisk = PhysicalDisks[0];

                // �����¶�ͼ��
                UpdateTemperatureCharts(systemInfo.CpuTemperature, systemInfo.GpuTemperature);

                // TPM (����)
                HasTpm = systemInfo.HasTpm;
                if (systemInfo.HasTpm)
                {
                    TpmManufacturer = string.IsNullOrWhiteSpace(systemInfo.TpmManufacturer) ? "δ֪������" : systemInfo.TpmManufacturer;
                    TpmManufacturerId = systemInfo.TpmManufacturerId;
                    TpmVersion = string.IsNullOrWhiteSpace(systemInfo.TpmVersion) ? "δ֪�汾" : systemInfo.TpmVersion;
                    TpmFirmwareVersion = systemInfo.TpmFirmwareVersion;
                    TpmStatus = string.IsNullOrWhiteSpace(systemInfo.TpmStatus) ? "δ֪״̬" : systemInfo.TpmStatus;
                    TpmEnabled = systemInfo.TpmEnabled;
                    TpmIsActivated = systemInfo.TpmIsActivated;
                    TpmIsOwned = systemInfo.TpmIsOwned;
                    TpmReady = systemInfo.TpmReady;
                    TpmTbsAvailable = systemInfo.TpmTbsAvailable;
                    TpmPhysicalPresenceRequired = systemInfo.TpmPhysicalPresenceRequired;
                    TpmSpecVersion = systemInfo.TpmSpecVersion;
                    TpmTbsVersion = systemInfo.TpmTbsVersion;
                    TpmErrorMessage = systemInfo.TpmErrorMessage;
                    TpmDetectionMethod = string.IsNullOrWhiteSpace(systemInfo.TpmDetectionMethod) ? "δ֪" : systemInfo.TpmDetectionMethod;
                    TpmWmiDetectionWorked = systemInfo.TpmWmiDetectionWorked;
                    TpmTbsDetectionWorked = systemInfo.TpmTbsDetectionWorked;
                }
                else
                {
                    TpmManufacturer = "δ��⵽TPM";
                    TpmVersion = "-";
                    TpmStatus = "��";
                    TpmEnabled = false;
                    TpmReady = false;
                    TpmIsActivated = false;
                    TpmIsOwned = false;
                    TpmTbsAvailable = false;
                    TpmPhysicalPresenceRequired = false;
                    TpmSpecVersion = 0;
                    TpmTbsVersion = 0;
                    TpmErrorMessage = string.Empty;
                    TpmDetectionMethod = "δ��⵽";
                    TpmWmiDetectionWorked = false;
                    TpmTbsDetectionWorked = false;
                }

                Log.Debug($"ϵͳ���ݸ������: CPU={CpuName}, �ڴ�ʹ����={MemoryUsagePercent:F1}%, GPU����={Gpus.Count}");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "����ϵͳ����ʱ��������");
                CpuName = $"?? ���ݸ���ʧ��: {ex.Message}";
            }
        }

        private void BuildOrUpdatePhysicalDisks(SystemInfo systemInfo)
        {
            try
            {
                // �Ƚ��� �����������к� -> ����View ӳ�䣬������������
                var map = PhysicalDisks.ToDictionary(p => p.Disk.SerialNumber, p => p);

                // ����Դ��ڵ�
                var alive = new HashSet<string>();

                foreach (var pd in systemInfo.PhysicalDisks)
                {
                    if (!map.TryGetValue(pd.SerialNumber, out var view))
                    {
                        view = new PhysicalDiskView { Disk = pd };
                        PhysicalDisks.Add(view);
                    }
                    else
                    {
                        // �������ã��������ؽ���
                        view.Disk = pd;
                    }
                    alive.Add(pd.SerialNumber);

                    // ����������б��������� Disks �� PhysicalDiskIndex ��Ӧ��
                    var partitions = systemInfo.Disks.Where(d => d.PhysicalDiskIndex >= 0 && d.PhysicalDiskIndex < systemInfo.PhysicalDisks.Count && systemInfo.PhysicalDisks[d.PhysicalDiskIndex].SerialNumber == pd.SerialNumber).ToList();

                    // ����ͬ�� view.Partitions
                    // ɾ�������ڵ�
                    for (int i = view.Partitions.Count - 1; i >= 0; i--)
                    {
                        var part = view.Partitions[i];
                        if (!partitions.Any(p => p.Letter == part.Letter))
                            view.Partitions.RemoveAt(i);
                    }
                    // ����/����
                    foreach (var part in partitions)
                    {
                        var existing = view.Partitions.FirstOrDefault(p => p.Letter == part.Letter);
                        if (existing == null)
                        {
                            view.Partitions.Add(part);
                        }
                        else
                        {
                            // ���������Ѿ����¹���ͬ���󣩣����ǲ�ͬʵ���������Ը��ƣ��������ͬʵ��
                        }
                    }
                }

                // �Ƴ��Ѳ����ڵ���������
                for (int i = PhysicalDisks.Count - 1; i >= 0; i--)
                {
                    if (!alive.Contains(PhysicalDisks[i].Disk.SerialNumber))
                        PhysicalDisks.RemoveAt(i);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "�����������̷���ʧ��");
            }
        }

        private double GetDiskUsagePercent(DiskData disk)
        {
            if (disk.TotalSize <= 0) return 0;
            return (double)disk.UsedSpace / disk.TotalSize * 100.0;
        }

        private string ValidateAndSetString(string? value, string fieldName)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                Log.Debug($"{fieldName} ����Ϊ�գ�ʹ��Ĭ��ֵ");
                return $"{fieldName} δ��⵽";
            }
            return value;
        }

        private int ValidateAndSetInt(int value, string fieldName)
        {
            if (value <= 0)
            {
                Log.Debug($"{fieldName} �����쳣: {value}��ʹ��Ĭ��ֵ0");
                return 0;
            }
            return value;
        }

        private double ValidateAndSetDouble(double value, string fieldName)
        {
            if (double.IsNaN(value) || double.IsInfinity(value) || value < 0)
            {
                Log.Debug($"{fieldName} �����쳣: {value}��ʹ��Ĭ��ֵ0");
                return 0.0;
            }
            return value;
        }

        private void UpdateCollection<T>(ObservableCollection<T> collection, List<T> newItems)
        {
            // �Ľ��ĸ��²��ԣ�ֻ�ڱ�Ҫʱ���¼���
            try
            {
                if (newItems == null)
                {
                    if (collection.Count > 0)
                    {
                        collection.Clear();
                    }
                    return;
                }

                if (collection.Count != newItems.Count || !collection.SequenceEqual(newItems))
                {
                    collection.Clear();
                    foreach (var item in newItems)
                    {
                        collection.Add(item);
                    }
                    
                    Log.Debug($"���¼���: {typeof(T).Name}, ����: {newItems.Count}");
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, $"���¼���ʱ��������: {typeof(T).Name}");
            }
        }

        private void UpdateTemperatureCharts(double cpuTemp, double gpuTemp)
        {
            try
            {
                // ��֤�¶����ݵ���Ч��
                if (cpuTemp > 0 && cpuTemp < 150) // ������CPU�¶ȷ�Χ
                {
                    _cpuTempData.Add(cpuTemp);
                }
                else if (_cpuTempData.Count == 0)
                {
                    _cpuTempData.Add(0); // ���û�����ݣ�����0�Ա���ͼ��������
                }

                if (gpuTemp > 0 && gpuTemp < 150) // ������GPU�¶ȷ�Χ
                {
                    _gpuTempData.Add(gpuTemp);
                }
                else if (_gpuTempData.Count == 0)
                {
                    _gpuTempData.Add(0);
                }

                // ����������ݵ�����
                while (_cpuTempData.Count > MAX_CHART_POINTS)
                    _cpuTempData.RemoveAt(0);

                while (_gpuTempData.Count > MAX_CHART_POINTS)
                    _gpuTempData.RemoveAt(0);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "�����¶�ͼ��ʱ��������");
            }
        }

        private string FormatBytes(ulong bytes)
        {
            if (bytes == 0) return "0 B";

            const ulong KB = 1024UL;
            const ulong MB = KB * KB;
            const ulong GB = MB * KB;
            const ulong TB = GB * KB;

            return bytes switch
            {
                >= TB => $"{(double)bytes / TB:F1} TB",
                >= GB => $"{(double)bytes / GB:F1} GB",
                >= MB => $"{(double)bytes / MB:F1} MB",
                >= KB => $"{(double)bytes / KB:F1} KB",
                _ => $"{bytes} B"
            };
        }

        public string FormatFrequency(double frequency)
        {
            if (frequency <= 0) return "δ֪";
            
            return frequency >= 1000 
                ? $"{frequency / 1000.0:F1} GHz" 
                : $"{frequency:F1} MHz";
        }

        public string FormatPercentage(double value)
        {
            if (double.IsNaN(value) || value < 0) return "δ֪";
            return $"{value:F1}%";
        }

        public string FormatNetworkSpeed(ulong speedBps)
        {
            if (speedBps == 0) return "δ����";

            const ulong Kbps = 1000UL;
            const ulong Mbps = Kbps * Kbps;
            const ulong Gbps = Mbps * Kbps;

            return speedBps switch
            {
                >= Gbps => $"{(double)speedBps / Gbps:F1} Gbps",
                >= Mbps => $"{(double)speedBps / Mbps:F1} Mbps",
                >= Kbps => $"{(double)speedBps / Kbps:F1} Kbps",
                _ => $"{speedBps} bps"
            };
        }

        [RelayCommand]
        private void ShowSmartDetails()
        {
            if (SelectedDisk != null)
            {
                // TODO: ʵ��SMART����Ի���
                Log.Information($"��ʾ���� {SelectedDisk.Letter}: ��SMART����");
            }
            else
            {
                Log.Warning("δѡ����̣��޷���ʾSMART����");
            }
        }

        [RelayCommand]
        private void Reconnect()
        {
            Log.Information("�û��ֶ�������������");
            _consecutiveErrors = 0;
            TryConnect();
        }

        protected override void OnPropertyChanged(PropertyChangedEventArgs e)
        {
            base.OnPropertyChanged(e);
            
            // �����������������Ա仯ʱ�����⴦���߼�
            if (e.PropertyName == nameof(IsConnected))
            {
                Log.Debug($"����״̬�仯: {IsConnected}");
            }
        }
    }
}