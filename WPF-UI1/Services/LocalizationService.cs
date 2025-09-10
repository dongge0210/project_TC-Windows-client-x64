using System;
using System.Collections.Generic;
using System.Globalization;
using System.Resources;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Serilog;

namespace WPF_UI1.Services
{
    /// <summary>
    /// 国际化服务
    /// </summary>
    public class LocalizationService : INotifyPropertyChanged
    {
        private static LocalizationService _instance;
        private static readonly object _lock = new object();
        private CultureInfo _currentCulture;
        private ResourceManager _resourceManager;
        private readonly Dictionary<string, ResourceManager> _resourceManagers;

        private LocalizationService()
        {
            _resourceManagers = new Dictionary<string, ResourceManager>();
            _currentCulture = CultureInfo.CurrentCulture;
            InitializeResourceManagers();
        }

        /// <summary>
        /// 单例实例
        /// </summary>
        public static LocalizationService Instance
        {
            get
            {
                if (_instance == null)
                {
                    lock (_lock)
                    {
                        if (_instance == null)
                        {
                            _instance = new LocalizationService();
                        }
                    }
                }
                return _instance;
            }
        }

        /// <summary>
        /// 当前文化信息
        /// </summary>
        public CultureInfo CurrentCulture
        {
            get => _currentCulture;
            set
            {
                if (_currentCulture != value)
                {
                    _currentCulture = value;
                    CultureInfo.CurrentCulture = value;
                    CultureInfo.CurrentUICulture = value;
                    OnPropertyChanged();
                    OnLanguageChanged?.Invoke(value);
                    Log.Information("语言已切换到: {Culture}", value.DisplayName);
                }
            }
        }

        /// <summary>
        /// 支持的语言列表
        /// </summary>
        public List<CultureInfo> SupportedCultures { get; } = new List<CultureInfo>
        {
            new CultureInfo("zh-CN"), // 简体中文
            new CultureInfo("zh-TW"), // 繁体中文
            new CultureInfo("en-US"), // 英语
            new CultureInfo("ja-JP"), // 日语
            new CultureInfo("ko-KR"), // 韩语
            new CultureInfo("ru-RU"), // 俄语
            new CultureInfo("de-DE"), // 德语
            new CultureInfo("fr-FR"), // 法语
            new CultureInfo("es-ES"), // 西班牙语
            new CultureInfo("pt-BR")  // 葡萄牙语
        };

        /// <summary>
        /// 语言更改事件
        /// </summary>
        public event Action<CultureInfo> OnLanguageChanged;

        /// <summary>
        /// 属性更改事件
        /// </summary>
        public event PropertyChangedEventHandler PropertyChanged;

        /// <summary>
        /// 初始化资源管理器
        /// </summary>
        private void InitializeResourceManagers()
        {
            try
            {
                // 主资源管理器
                _resourceManager = new ResourceManager("WPF_UI1.Resources.Localization.Strings", 
                    typeof(LocalizationService).Assembly);
                _resourceManagers["Main"] = _resourceManager;

                // 可以添加更多的资源管理器
                // _resourceManagers["UI"] = new ResourceManager("WPF_UI1.Resources.Localization.UI", typeof(LocalizationService).Assembly);
                // _resourceManagers["Messages"] = new ResourceManager("WPF_UI1.Resources.Localization.Messages", typeof(LocalizationService).Assembly);

                Log.Information("国际化资源管理器初始化完成");
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化资源管理器失败");
            }
        }

        /// <summary>
        /// 获取本地化字符串
        /// </summary>
        /// <param name="key">键</param>
        /// <param name="defaultValue">默认值</param>
        /// <returns>本地化字符串</returns>
        public string GetString(string key, string defaultValue = null)
        {
            return GetString("Main", key, defaultValue);
        }

        /// <summary>
        /// 获取本地化字符串
        /// </summary>
        /// <param name="resourceName">资源名称</param>
        /// <param name="key">键</param>
        /// <param name="defaultValue">默认值</param>
        /// <returns>本地化字符串</returns>
        public string GetString(string resourceName, string key, string defaultValue = null)
        {
            try
            {
                if (_resourceManagers.TryGetValue(resourceName, out var resourceManager))
                {
                    var value = resourceManager.GetString(key, _currentCulture);
                    return value ?? defaultValue ?? key;
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "获取本地化字符串失败: {Key}", key);
            }

            return defaultValue ?? key;
        }

        /// <summary>
        /// 获取格式化的本地化字符串
        /// </summary>
        /// <param name="key">键</param>
        /// <param name="args">格式化参数</param>
        /// <returns>格式化的本地化字符串</returns>
        public string GetFormattedString(string key, params object[] args)
        {
            var format = GetString(key);
            try
            {
                return string.Format(format, args);
            }
            catch (FormatException ex)
            {
                Log.Warning(ex, "格式化本地化字符串失败: {Key}", key);
                return format;
            }
        }

        /// <summary>
        /// 切换语言
        /// </summary>
        /// <param name="cultureName">文化名称</param>
        public void ChangeLanguage(string cultureName)
        {
            try
            {
                var culture = new CultureInfo(cultureName);
                CurrentCulture = culture;
            }
            catch (CultureNotFoundException ex)
            {
                Log.Error(ex, "不支持的文化信息: {CultureName}", cultureName);
            }
        }

        /// <summary>
        /// 获取当前语言的显示名称
        /// </summary>
        /// <returns>显示名称</returns>
        public string GetCurrentLanguageDisplayName()
        {
            return _currentCulture.DisplayName;
        }

        /// <summary>
        /// 检查是否支持指定的语言
        /// </summary>
        /// <param name="cultureName">文化名称</param>
        /// <returns>是否支持</returns>
        public bool IsSupportedLanguage(string cultureName)
        {
            return SupportedCultures.Exists(c => c.Name.Equals(cultureName, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// 获取系统默认语言（如果支持）
        /// </summary>
        /// <returns>默认语言</returns>
        public CultureInfo GetDefaultLanguage()
        {
            var systemCulture = CultureInfo.CurrentUICulture;
            var supportedCulture = SupportedCultures.Find(c => c.Name == systemCulture.Name);
            
            if (supportedCulture != null)
                return supportedCulture;

            // 尝试匹配语言代码（忽略地区）
            var languageCode = systemCulture.TwoLetterISOLanguageName;
            supportedCulture = SupportedCultures.Find(c => c.TwoLetterISOLanguageName == languageCode);
            
            return supportedCulture ?? new CultureInfo("zh-CN"); // 默认为简体中文
        }

        /// <summary>
        /// 保存语言设置
        /// </summary>
        public void SaveLanguageSetting()
        {
            try
            {
                // 这里可以保存到配置文件或注册表
                Properties.Settings.Default.Language = _currentCulture.Name;
                Properties.Settings.Default.Save();
                Log.Information("语言设置已保存: {Language}", _currentCulture.Name);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "保存语言设置失败");
            }
        }

        /// <summary>
        /// 加载语言设置
        /// </summary>
        public void LoadLanguageSetting()
        {
            try
            {
                var savedLanguage = Properties.Settings.Default.Language;
                if (!string.IsNullOrEmpty(savedLanguage) && IsSupportedLanguage(savedLanguage))
                {
                    ChangeLanguage(savedLanguage);
                    Log.Information("已加载保存的语言设置: {Language}", savedLanguage);
                }
                else
                {
                    CurrentCulture = GetDefaultLanguage();
                    Log.Information("使用默认语言: {Language}", CurrentCulture.Name);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "加载语言设置失败，使用默认语言");
                CurrentCulture = GetDefaultLanguage();
            }
        }

        /// <summary>
        /// 属性更改通知
        /// </summary>
        /// <param name="propertyName">属性名称</param>
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// WPF标记扩展，用于绑定本地化字符串
    /// </summary>
    public class LocalizeExtension : System.Windows.Markup.MarkupExtension
    {
        public string Key { get; set; }
        public string Default { get; set; }

        public LocalizeExtension(string key)
        {
            Key = key;
        }

        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            return LocalizationService.Instance.GetString(Key, Default);
        }
    }
}