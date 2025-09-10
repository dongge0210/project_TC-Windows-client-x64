using System.Configuration;

namespace WPF_UI1.Properties 
{
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "17.0.0.0")]
    internal sealed partial class Settings : global::System.Configuration.ApplicationSettingsBase 
    {
        private static Settings defaultInstance = ((Settings)(global::System.Configuration.ApplicationSettingsBase.Synchronized(new Settings())));
        
        public static Settings Default 
        {
            get 
            {
                return defaultInstance;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("zh-CN")]
        public string Language 
        {
            get 
            {
                return ((string)(this["Language"]));
            }
            set 
            {
                this["Language"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool EnableSystemTray 
        {
            get 
            {
                return ((bool)(this["EnableSystemTray"]));
            }
            set 
            {
                this["EnableSystemTray"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool EnableGlobalHotkeys 
        {
            get 
            {
                return ((bool)(this["EnableGlobalHotkeys"]));
            }
            set 
            {
                this["EnableGlobalHotkeys"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("True")]
        public bool EnableCrashDumps 
        {
            get 
            {
                return ((bool)(this["EnableCrashDumps"]));
            }
            set 
            {
                this["EnableCrashDumps"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1000")]
        public int RefreshInterval 
        {
            get 
            {
                return ((int)(this["RefreshInterval"]));
            }
            set 
            {
                this["RefreshInterval"] = value;
            }
        }
    }
}