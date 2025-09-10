using System.Windows;
using Serilog;
using System.IO;
using System;
using WPF_UI1.Services;
using WPF_UI1.Plugins;
using WPF_UI1.GPU;
using Microsoft.Extensions.DependencyInjection;

namespace WPF_UI1
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private SystemTrayService _systemTrayService;
        private GlobalHotkeyService _globalHotkeyService;
        private PluginManager _pluginManager;
        private GPUManager _gpuManager;
        private IServiceProvider _serviceProvider;

        protected override void OnStartup(StartupEventArgs e)
        {
            // 配置日志
            ConfigureLogging();
            
            Log.Information("=== 系统硬件监视器 WPF UI 启动 ===");
            Log.Information("应用程序版本: 1.0.0");
            Log.Information("运行时版本: {RuntimeVersion}", Environment.Version);
            Log.Information("操作系统: {OS}", Environment.OSVersion);

            // 初始化崩溃转储服务
            if (Properties.Settings.Default.EnableCrashDumps)
            {
                CrashDumpService.Initialize();
            }

            // 设置全局异常处理
            AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
            DispatcherUnhandledException += OnDispatcherUnhandledException;

            // 初始化国际化服务
            LocalizationService.Instance.LoadLanguageSetting();

            // 配置依赖注入
            ConfigureServices();

            // 初始化GPU管理器
            InitializeGPUManager();

            // 初始化插件管理器
            InitializePluginManager();

            base.OnStartup(e);
            
            // 在主窗口创建后初始化其他服务
            MainWindow.Loaded += OnMainWindowLoaded;
        }

        protected override void OnExit(ExitEventArgs e)
        {
            Log.Information("应用程序正在退出，退出代码: {ExitCode}", e.ApplicationExitCode);
            
            // 清理资源
            CleanupServices();
            
            Log.CloseAndFlush();
            base.OnExit(e);
        }

        /// <summary>
        /// 配置依赖注入服务
        /// </summary>
        private void ConfigureServices()
        {
            var services = new ServiceCollection();
            
            // 注册核心服务
            services.AddSingleton<LocalizationService>(LocalizationService.Instance);
            services.AddSingleton<SharedMemoryService>();
            
            // 注册GPU管理器
            services.AddSingleton<GPUManager>();
            
            _serviceProvider = services.BuildServiceProvider();
            Log.Information("依赖注入服务配置完成");
        }

        /// <summary>
        /// 初始化GPU管理器
        /// </summary>
        private void InitializeGPUManager()
        {
            try
            {
                _gpuManager = _serviceProvider.GetService<GPUManager>();
                _gpuManager.ScanGPUs();
                _gpuManager.StartMonitoring();
                Log.Information("GPU管理器初始化完成");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化GPU管理器失败");
            }
        }

        /// <summary>
        /// 初始化插件管理器
        /// </summary>
        private void InitializePluginManager()
        {
            try
            {
                _pluginManager = new PluginManager(_serviceProvider, Log.Logger);
                _pluginManager.LoadAllPlugins();
                _pluginManager.InitializeAllPlugins();
                _pluginManager.StartAllPlugins();
                Log.Information("插件管理器初始化完成");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化插件管理器失败");
            }
        }

        /// <summary>
        /// 主窗口加载完成事件
        /// </summary>
        private void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            var mainWindow = sender as MainWindow;
            if (mainWindow == null) return;

            try
            {
                // 初始化系统托盘
                if (Properties.Settings.Default.EnableSystemTray)
                {
                    _systemTrayService = new SystemTrayService(mainWindow);
                    _systemTrayService.ShowTrayIcon();
                    
                    // 处理窗口关闭事件
                    mainWindow.Closing += (s, args) =>
                    {
                        args.Cancel = true; // 取消关闭
                        _systemTrayService.OnMainWindowClosing();
                    };
                }

                // 初始化全局热键
                if (Properties.Settings.Default.EnableGlobalHotkeys)
                {
                    _globalHotkeyService = new GlobalHotkeyService(mainWindow);
                    _globalHotkeyService.OnHotkeyPressed += OnGlobalHotkeyPressed;
                }

                Log.Information("所有服务初始化完成");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化应用程序服务失败");
            }
        }

        /// <summary>
        /// 全局热键按下事件处理
        /// </summary>
        private void OnGlobalHotkeyPressed(string hotkeyName)
        {
            Log.Information("全局热键被触发: {HotkeyName}", hotkeyName);
            
            switch (hotkeyName)
            {
                case "RefreshData":
                    // 触发数据刷新
                    break;
                case "TakeScreenshot":
                    // 截图功能
                    break;
            }
        }

        /// <summary>
        /// 清理服务资源
        /// </summary>
        private void CleanupServices()
        {
            try
            {
                _globalHotkeyService?.Dispose();
                _systemTrayService?.Dispose();
                _pluginManager?.Dispose();
                _gpuManager?.Dispose();
                
                // 清理旧的转储文件
                if (Properties.Settings.Default.EnableCrashDumps)
                {
                    CrashDumpService.CleanupOldDumps();
                }
                
                Log.Information("应用程序服务清理完成");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "清理应用程序服务时发生错误");
            }
        }

        private void ConfigureLogging()
        {
            var logDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Logs");
            if (!Directory.Exists(logDirectory))
            {
                Directory.CreateDirectory(logDirectory);
            }

            var logFilePath = Path.Combine(logDirectory, "wpf_monitor.log");

            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Debug()
                .WriteTo.Console(
                    outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] {Message:lj}{NewLine}{Exception}")
                .WriteTo.File(
                    logFilePath,
                    rollingInterval: RollingInterval.Day,
                    retainedFileCountLimit: 7,
                    outputTemplate: "[{Timestamp:yyyy-MM-dd HH:mm:ss.fff zzz} {Level:u3}] {Message:lj}{NewLine}{Exception}")
                .CreateLogger();

            Log.Information("日志系统初始化完成，日志文件: {LogPath}", logFilePath);
        }

        private void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            var exception = e.ExceptionObject as Exception;
            Log.Fatal(exception, "发生未处理的异常，程序即将终止");
            
            // 生成崩溃转储
            if (Properties.Settings.Default.EnableCrashDumps)
            {
                CrashDumpService.GenerateCrashDump(exception);
            }
            
            MessageBox.Show(
                $"发生严重错误，程序将退出：\n\n{exception?.Message}\n\n详细信息已记录到日志文件。",
                "严重错误",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }

        private void OnDispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            Log.Error(e.Exception, "发生未处理的UI异常");
            
            // 生成崩溃转储
            if (Properties.Settings.Default.EnableCrashDumps)
            {
                CrashDumpService.GenerateCrashDump(e.Exception);
            }
            
            MessageBox.Show(
                $"发生UI错误：\n\n{e.Exception.Message}\n\n详细信息已记录到日志文件。",
                "UI错误",
                MessageBoxButton.OK,
                MessageBoxImage.Warning);

            // 标记异常已处理，防止程序崩溃
            e.Handled = true;
        }
    }
}
