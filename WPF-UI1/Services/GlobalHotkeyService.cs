using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using Serilog;

namespace WPF_UI1.Services
{
    /// <summary>
    /// 全局热键服务
    /// </summary>
    public class GlobalHotkeyService : IDisposable
    {
        #region Windows API

        [DllImport("user32.dll")]
        private static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);

        [DllImport("user32.dll")]
        private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

        private const int WM_HOTKEY = 0x0312;

        #endregion

        #region 修饰键枚举

        [Flags]
        public enum ModifierKeys : uint
        {
            None = 0,
            Alt = 1,
            Control = 2,
            Shift = 4,
            Win = 8
        }

        #endregion

        private readonly Dictionary<int, HotkeyInfo> _registeredHotkeys;
        private readonly Window _window;
        private HwndSource _hwndSource;
        private int _nextHotkeyId = 1;
        private bool _disposed = false;

        public GlobalHotkeyService(Window window)
        {
            _window = window;
            _registeredHotkeys = new Dictionary<int, HotkeyInfo>();
            
            // 等待窗口完全加载后初始化
            if (_window.IsLoaded)
            {
                Initialize();
            }
            else
            {
                _window.Loaded += OnWindowLoaded;
            }
        }

        /// <summary>
        /// 窗口加载完成事件
        /// </summary>
        private void OnWindowLoaded(object sender, RoutedEventArgs e)
        {
            Initialize();
            _window.Loaded -= OnWindowLoaded;
        }

        /// <summary>
        /// 初始化热键服务
        /// </summary>
        private void Initialize()
        {
            try
            {
                var windowInteropHelper = new WindowInteropHelper(_window);
                _hwndSource = HwndSource.FromHwnd(windowInteropHelper.Handle);
                _hwndSource.AddHook(WndProc);
                
                // 注册默认热键
                RegisterDefaultHotkeys();
                
                Log.Information("全局热键服务已初始化");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化全局热键服务失败");
            }
        }

        /// <summary>
        /// 注册默认热键
        /// </summary>
        private void RegisterDefaultHotkeys()
        {
            try
            {
                // Ctrl+Shift+M: 显示/隐藏主窗口
                RegisterHotkey(ModifierKeys.Control | ModifierKeys.Shift, Key.M, "ToggleMainWindow", 
                    () => {
                        if (_window.IsVisible && _window.WindowState != WindowState.Minimized)
                        {
                            _window.Hide();
                        }
                        else
                        {
                            _window.Show();
                            _window.WindowState = WindowState.Normal;
                            _window.Activate();
                        }
                    });

                // Ctrl+Shift+Q: 退出应用程序
                RegisterHotkey(ModifierKeys.Control | ModifierKeys.Shift, Key.Q, "ExitApplication",
                    () => {
                        Application.Current.Shutdown();
                    });

                // Ctrl+Shift+R: 刷新数据
                RegisterHotkey(ModifierKeys.Control | ModifierKeys.Shift, Key.R, "RefreshData",
                    () => {
                        // 触发数据刷新事件
                        OnHotkeyPressed?.Invoke("RefreshData");
                    });

                // Ctrl+Shift+S: 屏幕截图
                RegisterHotkey(ModifierKeys.Control | ModifierKeys.Shift, Key.S, "TakeScreenshot",
                    () => {
                        OnHotkeyPressed?.Invoke("TakeScreenshot");
                    });

                Log.Information("默认热键已注册");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "注册默认热键失败");
            }
        }

        /// <summary>
        /// 注册热键
        /// </summary>
        /// <param name="modifiers">修饰键</param>
        /// <param name="key">按键</param>
        /// <param name="name">热键名称</param>
        /// <param name="action">执行动作</param>
        /// <returns>热键ID，失败返回-1</returns>
        public int RegisterHotkey(ModifierKeys modifiers, Key key, string name, Action action)
        {
            try
            {
                if (_hwndSource == null)
                {
                    Log.Warning("热键服务未初始化，无法注册热键: {Name}", name);
                    return -1;
                }

                var virtualKey = KeyInterop.VirtualKeyFromKey(key);
                var hotkeyId = _nextHotkeyId++;

                if (RegisterHotKey(_hwndSource.Handle, hotkeyId, (uint)modifiers, (uint)virtualKey))
                {
                    var hotkeyInfo = new HotkeyInfo
                    {
                        Id = hotkeyId,
                        Modifiers = modifiers,
                        Key = key,
                        Name = name,
                        Action = action
                    };

                    _registeredHotkeys[hotkeyId] = hotkeyInfo;
                    
                    Log.Information("热键已注册: {Name} ({Modifiers}+{Key})", name, modifiers, key);
                    return hotkeyId;
                }
                else
                {
                    Log.Warning("注册热键失败: {Name} ({Modifiers}+{Key})", name, modifiers, key);
                    return -1;
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "注册热键异常: {Name}", name);
                return -1;
            }
        }

        /// <summary>
        /// 注销热键
        /// </summary>
        /// <param name="hotkeyId">热键ID</param>
        /// <returns>是否成功</returns>
        public bool UnregisterHotkey(int hotkeyId)
        {
            try
            {
                if (_hwndSource == null || !_registeredHotkeys.ContainsKey(hotkeyId))
                {
                    return false;
                }

                var success = UnregisterHotKey(_hwndSource.Handle, hotkeyId);
                if (success)
                {
                    var hotkeyInfo = _registeredHotkeys[hotkeyId];
                    _registeredHotkeys.Remove(hotkeyId);
                    
                    Log.Information("热键已注销: {Name}", hotkeyInfo.Name);
                }
                else
                {
                    Log.Warning("注销热键失败: {HotkeyId}", hotkeyId);
                }

                return success;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "注销热键异常: {HotkeyId}", hotkeyId);
                return false;
            }
        }

        /// <summary>
        /// 根据名称注销热键
        /// </summary>
        /// <param name="name">热键名称</param>
        /// <returns>是否成功</returns>
        public bool UnregisterHotkey(string name)
        {
            foreach (var kvp in _registeredHotkeys)
            {
                if (kvp.Value.Name == name)
                {
                    return UnregisterHotkey(kvp.Key);
                }
            }
            
            Log.Warning("未找到名为 {Name} 的热键", name);
            return false;
        }

        /// <summary>
        /// 获取已注册的热键列表
        /// </summary>
        /// <returns>热键信息列表</returns>
        public List<HotkeyInfo> GetRegisteredHotkeys()
        {
            return new List<HotkeyInfo>(_registeredHotkeys.Values);
        }

        /// <summary>
        /// 检查热键是否已注册
        /// </summary>
        /// <param name="modifiers">修饰键</param>
        /// <param name="key">按键</param>
        /// <returns>是否已注册</returns>
        public bool IsHotkeyRegistered(ModifierKeys modifiers, Key key)
        {
            foreach (var hotkeyInfo in _registeredHotkeys.Values)
            {
                if (hotkeyInfo.Modifiers == modifiers && hotkeyInfo.Key == key)
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// 热键按下事件
        /// </summary>
        public event Action<string> OnHotkeyPressed;

        /// <summary>
        /// 窗口消息处理
        /// </summary>
        /// <param name="hwnd">窗口句柄</param>
        /// <param name="msg">消息类型</param>
        /// <param name="wParam">消息参数</param>
        /// <param name="lParam">消息参数</param>
        /// <param name="handled">是否已处理</param>
        /// <returns>处理结果</returns>
        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == WM_HOTKEY)
            {
                var hotkeyId = wParam.ToInt32();
                if (_registeredHotkeys.TryGetValue(hotkeyId, out var hotkeyInfo))
                {
                    try
                    {
                        Log.Debug("热键被触发: {Name}", hotkeyInfo.Name);
                        hotkeyInfo.Action?.Invoke();
                        handled = true;
                    }
                    catch (Exception ex)
                    {
                        Log.Error(ex, "执行热键动作失败: {Name}", hotkeyInfo.Name);
                    }
                }
            }

            return IntPtr.Zero;
        }

        /// <summary>
        /// 获取热键描述字符串
        /// </summary>
        /// <param name="modifiers">修饰键</param>
        /// <param name="key">按键</param>
        /// <returns>描述字符串</returns>
        public static string GetHotkeyDescription(ModifierKeys modifiers, Key key)
        {
            var parts = new List<string>();

            if (modifiers.HasFlag(ModifierKeys.Control))
                parts.Add("Ctrl");
            if (modifiers.HasFlag(ModifierKeys.Alt))
                parts.Add("Alt");
            if (modifiers.HasFlag(ModifierKeys.Shift))
                parts.Add("Shift");
            if (modifiers.HasFlag(ModifierKeys.Win))
                parts.Add("Win");

            parts.Add(key.ToString());

            return string.Join("+", parts);
        }

        /// <summary>
        /// 释放资源
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                try
                {
                    // 注销所有热键
                    var hotkeyIds = new List<int>(_registeredHotkeys.Keys);
                    foreach (var hotkeyId in hotkeyIds)
                    {
                        UnregisterHotkey(hotkeyId);
                    }

                    // 移除窗口消息钩子
                    _hwndSource?.RemoveHook(WndProc);
                    
                    _disposed = true;
                    Log.Information("全局热键服务已释放");
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "释放全局热键服务时发生错误");
                }
            }
        }
    }

    /// <summary>
    /// 热键信息
    /// </summary>
    public class HotkeyInfo
    {
        /// <summary>
        /// 热键ID
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// 修饰键
        /// </summary>
        public GlobalHotkeyService.ModifierKeys Modifiers { get; set; }

        /// <summary>
        /// 按键
        /// </summary>
        public Key Key { get; set; }

        /// <summary>
        /// 热键名称
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// 执行动作
        /// </summary>
        public Action Action { get; set; }

        /// <summary>
        /// 获取热键描述
        /// </summary>
        /// <returns>描述字符串</returns>
        public string GetDescription()
        {
            return GlobalHotkeyService.GetHotkeyDescription(Modifiers, Key);
        }
    }
}