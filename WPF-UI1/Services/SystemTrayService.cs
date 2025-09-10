using System;
using System.Drawing;
using System.Windows.Forms;
using System.Windows;
using Serilog;
using WPF_UI1.Services;

namespace WPF_UI1.Services
{
    /// <summary>
    /// 系统托盘服务
    /// </summary>
    public class SystemTrayService : IDisposable
    {
        private NotifyIcon _notifyIcon;
        private ContextMenuStrip _contextMenu;
        private readonly Window _mainWindow;
        private readonly LocalizationService _localizationService;
        private bool _disposed = false;

        public SystemTrayService(Window mainWindow)
        {
            _mainWindow = mainWindow;
            _localizationService = LocalizationService.Instance;
            
            InitializeTrayIcon();
            InitializeContextMenu();
            
            // 订阅语言更改事件
            _localizationService.OnLanguageChanged += OnLanguageChanged;
            
            Log.Information("系统托盘服务已初始化");
        }

        /// <summary>
        /// 初始化托盘图标
        /// </summary>
        private void InitializeTrayIcon()
        {
            _notifyIcon = new NotifyIcon
            {
                Icon = CreateTrayIcon(),
                Text = _localizationService.GetString("AppTitle", "系统硬件监视器"),
                Visible = false
            };

            // 双击事件
            _notifyIcon.MouseDoubleClick += OnTrayIconDoubleClick;
            
            // 气球提示点击事件
            _notifyIcon.BalloonTipClicked += OnBalloonTipClicked;
        }

        /// <summary>
        /// 创建托盘图标
        /// </summary>
        /// <returns>图标</returns>
        private Icon CreateTrayIcon()
        {
            try
            {
                // 尝试从资源加载图标
                var iconStream = System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/Resources/app.ico"));
                if (iconStream != null)
                {
                    return new Icon(iconStream.Stream);
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "加载应用程序图标失败，使用默认图标");
            }

            // 创建简单的默认图标
            return CreateDefaultIcon();
        }

        /// <summary>
        /// 创建默认图标
        /// </summary>
        /// <returns>默认图标</returns>
        private Icon CreateDefaultIcon()
        {
            var bitmap = new Bitmap(16, 16);
            using (var graphics = Graphics.FromImage(bitmap))
            {
                // 绘制简单的监视器图标
                graphics.Clear(Color.Transparent);
                graphics.FillRectangle(Brushes.DarkBlue, 2, 3, 12, 8);
                graphics.FillRectangle(Brushes.LightBlue, 3, 4, 10, 6);
                graphics.FillRectangle(Brushes.DarkGray, 6, 11, 4, 2);
                graphics.FillRectangle(Brushes.DarkGray, 4, 13, 8, 2);
            }

            var handle = bitmap.GetHicon();
            return Icon.FromHandle(handle);
        }

        /// <summary>
        /// 初始化上下文菜单
        /// </summary>
        private void InitializeContextMenu()
        {
            _contextMenu = new ContextMenuStrip();
            UpdateContextMenu();
            _notifyIcon.ContextMenuStrip = _contextMenu;
        }

        /// <summary>
        /// 更新上下文菜单
        /// </summary>
        private void UpdateContextMenu()
        {
            _contextMenu.Items.Clear();

            // 显示/隐藏主窗口
            var showHideItem = new ToolStripMenuItem
            {
                Text = _mainWindow.IsVisible 
                    ? _localizationService.GetString("Tray_Hide", "隐藏")
                    : _localizationService.GetString("Tray_Show", "显示"),
                Font = new Font(_contextMenu.Font, FontStyle.Bold)
            };
            showHideItem.Click += OnShowHideClick;
            _contextMenu.Items.Add(showHideItem);

            _contextMenu.Items.Add(new ToolStripSeparator());

            // 语言子菜单
            var languageMenu = new ToolStripMenuItem
            {
                Text = _localizationService.GetString("Menu_Language", "语言")
            };

            foreach (var culture in _localizationService.SupportedCultures)
            {
                var languageItem = new ToolStripMenuItem
                {
                    Text = culture.DisplayName,
                    Checked = culture.Name == _localizationService.CurrentCulture.Name,
                    Tag = culture.Name
                };
                languageItem.Click += OnLanguageItemClick;
                languageMenu.DropDownItems.Add(languageItem);
            }

            _contextMenu.Items.Add(languageMenu);

            _contextMenu.Items.Add(new ToolStripSeparator());

            // 设置
            var settingsItem = new ToolStripMenuItem
            {
                Text = _localizationService.GetString("Menu_Settings", "设置")
            };
            settingsItem.Click += OnSettingsClick;
            _contextMenu.Items.Add(settingsItem);

            // 关于
            var aboutItem = new ToolStripMenuItem
            {
                Text = _localizationService.GetString("Tray_About", "关于")
            };
            aboutItem.Click += OnAboutClick;
            _contextMenu.Items.Add(aboutItem);

            _contextMenu.Items.Add(new ToolStripSeparator());

            // 退出
            var exitItem = new ToolStripMenuItem
            {
                Text = _localizationService.GetString("Tray_Exit", "退出")
            };
            exitItem.Click += OnExitClick;
            _contextMenu.Items.Add(exitItem);
        }

        /// <summary>
        /// 显示托盘图标
        /// </summary>
        public void ShowTrayIcon()
        {
            _notifyIcon.Visible = true;
            Log.Information("系统托盘图标已显示");
        }

        /// <summary>
        /// 隐藏托盘图标
        /// </summary>
        public void HideTrayIcon()
        {
            _notifyIcon.Visible = false;
            Log.Information("系统托盘图标已隐藏");
        }

        /// <summary>
        /// 显示气球提示
        /// </summary>
        /// <param name="title">标题</param>
        /// <param name="message">消息</param>
        /// <param name="icon">图标类型</param>
        /// <param name="timeout">超时时间（毫秒）</param>
        public void ShowBalloonTip(string title, string message, ToolTipIcon icon = ToolTipIcon.Info, int timeout = 3000)
        {
            if (_notifyIcon.Visible)
            {
                _notifyIcon.ShowBalloonTip(timeout, title, message, icon);
                Log.Information("已显示气球提示: {Title} - {Message}", title, message);
            }
        }

        /// <summary>
        /// 托盘图标双击事件
        /// </summary>
        private void OnTrayIconDoubleClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                ShowOrHideMainWindow();
            }
        }

        /// <summary>
        /// 显示或隐藏主窗口
        /// </summary>
        private void ShowOrHideMainWindow()
        {
            if (_mainWindow.IsVisible)
            {
                _mainWindow.Hide();
                Log.Information("主窗口已隐藏到系统托盘");
            }
            else
            {
                _mainWindow.Show();
                _mainWindow.WindowState = WindowState.Normal;
                _mainWindow.Activate();
                Log.Information("主窗口已从系统托盘恢复");
            }
            
            // 更新上下文菜单
            UpdateContextMenu();
        }

        /// <summary>
        /// 显示/隐藏菜单项点击事件
        /// </summary>
        private void OnShowHideClick(object sender, EventArgs e)
        {
            ShowOrHideMainWindow();
        }

        /// <summary>
        /// 语言菜单项点击事件
        /// </summary>
        private void OnLanguageItemClick(object sender, EventArgs e)
        {
            if (sender is ToolStripMenuItem item && item.Tag is string cultureName)
            {
                _localizationService.ChangeLanguage(cultureName);
                _localizationService.SaveLanguageSetting();
                
                // 显示语言切换提示
                var title = _localizationService.GetString("AppTitle", "系统硬件监视器");
                var message = $"语言已切换为: {_localizationService.CurrentCulture.DisplayName}";
                ShowBalloonTip(title, message, ToolTipIcon.Info);
            }
        }

        /// <summary>
        /// 设置菜单项点击事件
        /// </summary>
        private void OnSettingsClick(object sender, EventArgs e)
        {
            // TODO: 打开设置窗口
            Log.Information("设置菜单被点击");
        }

        /// <summary>
        /// 关于菜单项点击事件
        /// </summary>
        private void OnAboutClick(object sender, EventArgs e)
        {
            var title = _localizationService.GetString("Tray_About", "关于");
            var message = $"{_localizationService.GetString("AppTitle", "系统硬件监视器")}\n" +
                         $"{_localizationService.GetString("AppDescription", "实时硬件监控工具")}\n" +
                         $"版本: 1.0.0";
            
            System.Windows.MessageBox.Show(message, title, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        /// <summary>
        /// 退出菜单项点击事件
        /// </summary>
        private void OnExitClick(object sender, EventArgs e)
        {
            Log.Information("用户从系统托盘选择退出");
            System.Windows.Application.Current.Shutdown();
        }

        /// <summary>
        /// 气球提示点击事件
        /// </summary>
        private void OnBalloonTipClicked(object sender, EventArgs e)
        {
            // 点击气球提示时显示主窗口
            if (!_mainWindow.IsVisible)
            {
                ShowOrHideMainWindow();
            }
        }

        /// <summary>
        /// 语言更改事件处理
        /// </summary>
        /// <param name="newCulture">新的文化信息</param>
        private void OnLanguageChanged(System.Globalization.CultureInfo newCulture)
        {
            // 更新托盘图标提示文本
            _notifyIcon.Text = _localizationService.GetString("AppTitle", "系统硬件监视器");
            
            // 更新上下文菜单
            UpdateContextMenu();
            
            Log.Information("系统托盘已更新语言设置: {Culture}", newCulture.DisplayName);
        }

        /// <summary>
        /// 处理主窗口关闭事件
        /// </summary>
        public void OnMainWindowClosing()
        {
            // 隐藏到托盘而不是真正关闭
            _mainWindow.Hide();
            
            var title = _localizationService.GetString("AppTitle", "系统硬件监视器");
            var message = "应用程序已最小化到系统托盘，双击图标可恢复窗口";
            ShowBalloonTip(title, message, ToolTipIcon.Info);
        }

        /// <summary>
        /// 释放资源
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                if (_localizationService != null)
                {
                    _localizationService.OnLanguageChanged -= OnLanguageChanged;
                }

                _notifyIcon?.Dispose();
                _contextMenu?.Dispose();
                
                _disposed = true;
                Log.Information("系统托盘服务已释放");
            }
        }
    }
}