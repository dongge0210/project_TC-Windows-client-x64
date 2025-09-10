using System;
using System.Collections.Generic;

namespace WPF_UI1.Plugins
{
    /// <summary>
    /// 插件接口
    /// </summary>
    public interface IPlugin
    {
        /// <summary>
        /// 插件名称
        /// </summary>
        string Name { get; }

        /// <summary>
        /// 插件版本
        /// </summary>
        Version Version { get; }

        /// <summary>
        /// 插件描述
        /// </summary>
        string Description { get; }

        /// <summary>
        /// 插件作者
        /// </summary>
        string Author { get; }

        /// <summary>
        /// 插件是否启用
        /// </summary>
        bool IsEnabled { get; set; }

        /// <summary>
        /// 插件初始化
        /// </summary>
        /// <param name="context">插件上下文</param>
        void Initialize(IPluginContext context);

        /// <summary>
        /// 插件启动
        /// </summary>
        void Start();

        /// <summary>
        /// 插件停止
        /// </summary>
        void Stop();

        /// <summary>
        /// 插件销毁
        /// </summary>
        void Dispose();
    }

    /// <summary>
    /// 插件上下文接口
    /// </summary>
    public interface IPluginContext
    {
        /// <summary>
        /// 应用程序服务提供者
        /// </summary>
        IServiceProvider ServiceProvider { get; }

        /// <summary>
        /// 配置数据
        /// </summary>
        IDictionary<string, object> Configuration { get; }

        /// <summary>
        /// 日志记录器
        /// </summary>
        Serilog.ILogger Logger { get; }

        /// <summary>
        /// 获取服务
        /// </summary>
        /// <typeparam name="T">服务类型</typeparam>
        /// <returns>服务实例</returns>
        T GetService<T>();

        /// <summary>
        /// 发送消息给其他插件
        /// </summary>
        /// <param name="message">消息</param>
        void SendMessage(IPluginMessage message);

        /// <summary>
        /// 订阅消息
        /// </summary>
        /// <typeparam name="T">消息类型</typeparam>
        /// <param name="handler">处理器</param>
        void SubscribeMessage<T>(Action<T> handler) where T : IPluginMessage;
    }

    /// <summary>
    /// 插件消息接口
    /// </summary>
    public interface IPluginMessage
    {
        /// <summary>
        /// 消息类型
        /// </summary>
        string MessageType { get; }

        /// <summary>
        /// 发送者
        /// </summary>
        string Sender { get; }

        /// <summary>
        /// 时间戳
        /// </summary>
        DateTime Timestamp { get; }

        /// <summary>
        /// 消息数据
        /// </summary>
        object Data { get; }
    }

    /// <summary>
    /// 硬件监控插件接口
    /// </summary>
    public interface IHardwareMonitorPlugin : IPlugin
    {
        /// <summary>
        /// 支持的硬件类型
        /// </summary>
        HardwareType[] SupportedHardwareTypes { get; }

        /// <summary>
        /// 获取硬件数据
        /// </summary>
        /// <param name="hardwareType">硬件类型</param>
        /// <returns>硬件数据</returns>
        object GetHardwareData(HardwareType hardwareType);

        /// <summary>
        /// 监控数据更新事件
        /// </summary>
        event EventHandler<HardwareDataUpdatedEventArgs> DataUpdated;
    }

    /// <summary>
    /// 硬件类型枚举
    /// </summary>
    public enum HardwareType
    {
        CPU,
        GPU,
        Memory,
        Storage,
        Network,
        Motherboard,
        PowerSupply,
        Cooling,
        Unknown
    }

    /// <summary>
    /// 硬件数据更新事件参数
    /// </summary>
    public class HardwareDataUpdatedEventArgs : EventArgs
    {
        public HardwareType HardwareType { get; set; }
        public object Data { get; set; }
        public DateTime Timestamp { get; set; }
    }

    /// <summary>
    /// UI插件接口
    /// </summary>
    public interface IUIPlugin : IPlugin
    {
        /// <summary>
        /// 获取UI元素
        /// </summary>
        /// <returns>UI元素</returns>
        System.Windows.FrameworkElement GetUIElement();

        /// <summary>
        /// UI位置
        /// </summary>
        UIPosition Position { get; }
    }

    /// <summary>
    /// UI位置枚举
    /// </summary>
    public enum UIPosition
    {
        MainWindow,
        StatusBar,
        SystemTray,
        ContextMenu,
        ToolBar
    }

    /// <summary>
    /// 插件元数据特性
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    public class PluginMetadataAttribute : Attribute
    {
        public string Name { get; set; }
        public string Version { get; set; }
        public string Description { get; set; }
        public string Author { get; set; }
        public string MinimumVersion { get; set; }
        public string[] Dependencies { get; set; }
    }
}