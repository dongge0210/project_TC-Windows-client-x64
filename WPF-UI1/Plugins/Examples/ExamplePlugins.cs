using System;
using System.Windows;
using System.Windows.Controls;
using WPF_UI1.Plugins;

namespace WPF_UI1.Plugins.Examples
{
    /// <summary>
    /// 示例硬件监控插件
    /// </summary>
    [PluginMetadata(
        Name = "示例硬件监控插件",
        Version = "1.0.0",
        Description = "演示如何创建硬件监控插件",
        Author = "TCMT Team",
        MinimumVersion = "1.0.0"
    )]
    public class ExampleHardwarePlugin : IHardwareMonitorPlugin, IUIPlugin, IDisposable
    {
        private IPluginContext _context;
        private bool _disposed = false;

        #region IPlugin 实现

        public string Name => "示例硬件监控插件";
        public Version Version => new Version(1, 0, 0);
        public string Description => "演示如何创建硬件监控插件的示例";
        public string Author => "TCMT Team";
        public bool IsEnabled { get; set; } = true;

        public void Initialize(IPluginContext context)
        {
            _context = context;
            _context.Logger.Information("示例硬件监控插件已初始化");
            
            // 订阅消息
            _context.SubscribeMessage<ExampleMessage>(OnExampleMessage);
        }

        public void Start()
        {
            _context.Logger.Information("示例硬件监控插件已启动");
            
            // 发送启动消息
            _context.SendMessage(new ExampleMessage
            {
                MessageType = "PluginStarted",
                Sender = Name,
                Timestamp = DateTime.Now,
                Data = "插件已成功启动"
            });
        }

        public void Stop()
        {
            _context.Logger.Information("示例硬件监控插件已停止");
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                _context.Logger.Information("示例硬件监控插件资源已释放");
                _disposed = true;
            }
        }

        #endregion

        #region IHardwareMonitorPlugin 实现

        public HardwareType[] SupportedHardwareTypes => new[] 
        { 
            HardwareType.CPU, 
            HardwareType.Memory 
        };

        public event EventHandler<HardwareDataUpdatedEventArgs> DataUpdated;

        public object GetHardwareData(HardwareType hardwareType)
        {
            switch (hardwareType)
            {
                case HardwareType.CPU:
                    return new
                    {
                        Temperature = 45.5f,
                        Usage = 25.3f,
                        Frequency = 3200.0f
                    };
                case HardwareType.Memory:
                    return new
                    {
                        Usage = 65.2f,
                        Available = 8192L,
                        Used = 4096L
                    };
                default:
                    return null;
            }
        }

        #endregion

        #region IUIPlugin 实现

        public UIPosition Position => UIPosition.MainWindow;

        public FrameworkElement GetUIElement()
        {
            var panel = new StackPanel
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(10)
            };

            panel.Children.Add(new TextBlock
            {
                Text = "示例插件UI",
                FontWeight = FontWeights.Bold,
                Margin = new Thickness(0, 0, 0, 5)
            });

            panel.Children.Add(new TextBlock
            {
                Text = $"插件名称: {Name}",
                Margin = new Thickness(0, 2)
            });

            panel.Children.Add(new TextBlock
            {
                Text = $"版本: {Version}",
                Margin = new Thickness(0, 2)
            });

            panel.Children.Add(new TextBlock
            {
                Text = $"状态: {(IsEnabled ? "已启用" : "已禁用")}",
                Margin = new Thickness(0, 2)
            });

            var button = new Button
            {
                Content = "测试按钮",
                Margin = new Thickness(0, 5, 0, 0)
            };
            button.Click += (s, e) =>
            {
                MessageBox.Show("插件按钮被点击！", "示例插件", MessageBoxButton.OK, MessageBoxImage.Information);
            };
            panel.Children.Add(button);

            return panel;
        }

        #endregion

        #region 私有方法

        /// <summary>
        /// 处理示例消息
        /// </summary>
        private void OnExampleMessage(ExampleMessage message)
        {
            _context.Logger.Information("收到示例消息: {Message}", message.Data);
        }

        /// <summary>
        /// 触发数据更新事件
        /// </summary>
        private void TriggerDataUpdate(HardwareType hardwareType, object data)
        {
            DataUpdated?.Invoke(this, new HardwareDataUpdatedEventArgs
            {
                HardwareType = hardwareType,
                Data = data,
                Timestamp = DateTime.Now
            });
        }

        #endregion
    }

    /// <summary>
    /// 示例消息类
    /// </summary>
    public class ExampleMessage : IPluginMessage
    {
        public string MessageType { get; set; }
        public string Sender { get; set; }
        public DateTime Timestamp { get; set; }
        public object Data { get; set; }
    }

    /// <summary>
    /// 示例UI插件
    /// </summary>
    [PluginMetadata(
        Name = "示例UI插件",
        Version = "1.0.0",
        Description = "演示如何创建UI插件",
        Author = "TCMT Team"
    )]
    public class ExampleUIPlugin : IUIPlugin, IDisposable
    {
        private IPluginContext _context;
        private bool _disposed = false;

        #region IPlugin 实现

        public string Name => "示例UI插件";
        public Version Version => new Version(1, 0, 0);
        public string Description => "演示如何创建UI插件的示例";
        public string Author => "TCMT Team";
        public bool IsEnabled { get; set; } = true;

        public void Initialize(IPluginContext context)
        {
            _context = context;
            _context.Logger.Information("示例UI插件已初始化");
        }

        public void Start()
        {
            _context.Logger.Information("示例UI插件已启动");
        }

        public void Stop()
        {
            _context.Logger.Information("示例UI插件已停止");
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                _context.Logger.Information("示例UI插件资源已释放");
                _disposed = true;
            }
        }

        #endregion

        #region IUIPlugin 实现

        public UIPosition Position => UIPosition.StatusBar;

        public FrameworkElement GetUIElement()
        {
            return new TextBlock
            {
                Text = $"[{Name}] 运行正常",
                Margin = new Thickness(5, 0),
                VerticalAlignment = VerticalAlignment.Center
            };
        }

        #endregion
    }
}