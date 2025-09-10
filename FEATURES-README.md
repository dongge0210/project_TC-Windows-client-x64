# TCMT Windows Client 功能增强说明

## 概述

本次更新为TCMT Windows Client添加了以下核心功能：

1. **崩溃转储 (Crash Dumps)** - 自动生成崩溃转储文件用于调试
2. **插件框架 (Plugin Framework)** - 可扩展的插件系统
3. **国际化 (i18n)** - 多语言支持
4. **多语言系统托盘** - 带多语言支持的系统托盘
5. **全局热键** - 全局快捷键支持
6. **GPU抽象层** - 可扩展的GPU监控抽象框架

## 功能详细说明

### 1. 崩溃转储功能

**文件位置**: `WPF-UI1/Services/CrashDumpService.cs`

**功能特性**:
- 自动检测未处理异常并生成转储文件
- 支持Windows平台的MiniDump和跨平台的异常信息转储
- 自动清理30天以上的旧转储文件
- 详细的系统环境信息记录

**使用方法**:
```csharp
// 在应用启动时初始化
CrashDumpService.Initialize();

// 手动生成转储文件
var dumpPath = CrashDumpService.GenerateCrashDump(exception);
```

### 2. 插件框架

**核心文件**:
- `WPF-UI1/Plugins/IPlugin.cs` - 插件接口定义
- `WPF-UI1/Plugins/PluginManager.cs` - 插件管理器
- `WPF-UI1/Plugins/Examples/ExamplePlugins.cs` - 示例插件

**支持的插件类型**:
- **IPlugin** - 基础插件接口
- **IHardwareMonitorPlugin** - 硬件监控插件
- **IUIPlugin** - UI插件

**创建插件示例**:
```csharp
[PluginMetadata(Name = "我的插件", Version = "1.0.0")]
public class MyPlugin : IPlugin
{
    public void Initialize(IPluginContext context) { /* 初始化逻辑 */ }
    public void Start() { /* 启动逻辑 */ }
    public void Stop() { /* 停止逻辑 */ }
    // ... 其他接口实现
}
```

### 3. 国际化 (i18n) 支持

**核心文件**:
- `WPF-UI1/Services/LocalizationService.cs` - 国际化服务
- `WPF-UI1/Resources/Localization/Strings.resx` - 中文资源文件
- `WPF-UI1/Resources/Localization/Strings.en-US.resx` - 英文资源文件

**支持语言**:
- 简体中文 (zh-CN)
- 繁体中文 (zh-TW)
- 英语 (en-US)
- 日语 (ja-JP)
- 韩语 (ko-KR)
- 俄语 (ru-RU)
- 德语 (de-DE)
- 法语 (fr-FR)
- 西班牙语 (es-ES)
- 葡萄牙语 (pt-BR)

**使用方法**:
```csharp
// 获取本地化字符串
var text = LocalizationService.Instance.GetString("CPU_Name", "名称:");

// 切换语言
LocalizationService.Instance.ChangeLanguage("en-US");

// 在XAML中使用
Text="{local:Localize CPU_Name}"
```

### 4. 系统托盘功能

**文件位置**: `WPF-UI1/Services/SystemTrayService.cs`

**功能特性**:
- 自定义托盘图标和上下文菜单
- 多语言支持
- 气球提示通知
- 语言切换菜单
- 窗口显示/隐藏切换

**功能说明**:
- 双击托盘图标显示/隐藏主窗口
- 右键菜单提供语言切换、设置、关于、退出等选项
- 支持最小化到托盘而不退出程序

### 5. 全局热键

**文件位置**: `WPF-UI1/Services/GlobalHotkeyService.cs`

**默认热键**:
- `Ctrl+Shift+M` - 显示/隐藏主窗口
- `Ctrl+Shift+Q` - 退出应用程序
- `Ctrl+Shift+R` - 刷新数据
- `Ctrl+Shift+S` - 截图功能

**使用方法**:
```csharp
// 注册热键
var hotkeyId = service.RegisterHotkey(
    ModifierKeys.Control | ModifierKeys.Shift, 
    Key.F1, 
    "MyHotkey", 
    () => { /* 执行动作 */ });

// 注销热键
service.UnregisterHotkey(hotkeyId);
```

### 6. GPU抽象层

**核心文件**:
- `WPF-UI1/GPU/IGPUProvider.cs` - GPU提供者接口
- `WPF-UI1/GPU/GPUManager.cs` - GPU管理器

**抽象层组件**:
- **IGPUInfo** - GPU基本信息接口
- **IGPUMonitorData** - GPU监控数据接口  
- **IGPUProvider** - GPU数据提供者接口
- **IGPUOverclock** - GPU超频接口
- **IGPUProfile** - GPU配置文件接口

**使用方法**:
```csharp
// 扫描GPU
gpuManager.ScanGPUs();

// 获取GPU列表
var gpus = gpuManager.DetectedGPUs;

// 获取监控数据
var data = gpuManager.GetMonitorData(deviceId);

// 开始监控
gpuManager.StartMonitoring();
```

## 配置设置

应用程序支持以下配置选项（存储在用户设置中）:

- `Language` - 界面语言 (默认: zh-CN)
- `EnableSystemTray` - 启用系统托盘 (默认: True)
- `EnableGlobalHotkeys` - 启用全局热键 (默认: True)
- `EnableCrashDumps` - 启用崩溃转储 (默认: True)
- `RefreshInterval` - 数据刷新间隔毫秒 (默认: 1000)

## 目录结构

```
WPF-UI1/
├── GPU/                    # GPU抽象层
│   ├── IGPUProvider.cs
│   └── GPUManager.cs
├── Plugins/                # 插件框架
│   ├── IPlugin.cs
│   ├── PluginManager.cs
│   └── Examples/
│       └── ExamplePlugins.cs
├── Resources/              # 资源文件
│   └── Localization/
│       ├── Strings.resx
│       └── Strings.en-US.resx
├── Services/               # 核心服务
│   ├── CrashDumpService.cs
│   ├── LocalizationService.cs
│   ├── SystemTrayService.cs
│   └── GlobalHotkeyService.cs
└── Properties/             # 应用程序配置
    └── Settings.Designer.cs
```

## 开发指南

### 添加新语言

1. 在 `Resources/Localization/` 目录创建新的 `.resx` 文件
2. 文件命名格式: `Strings.{culture}.resx` (如: `Strings.ja-JP.resx`)
3. 在 `LocalizationService.cs` 中添加到 `SupportedCultures` 列表
4. 翻译所有字符串资源

### 创建插件

1. 实现 `IPlugin` 接口及其他所需接口
2. 添加 `PluginMetadataAttribute` 特性
3. 编译为 `.dll` 文件
4. 放置到应用程序 `Plugins` 目录
5. 重启应用程序自动加载

### 扩展GPU支持

1. 实现 `IGPUProvider` 接口
2. 在 `GPUManager.InitializeProviders()` 中注册提供者
3. 实现硬件检测和监控逻辑

## 技术要求

- .NET 8.0 或更高版本
- Windows 10/11 (某些功能如全局热键需要Windows平台)
- WPF 和 Windows Forms 支持

## 依赖包

- Serilog - 日志记录
- MaterialDesignThemes - UI主题
- CommunityToolkit.Mvvm - MVVM框架
- Microsoft.Extensions.DependencyInjection - 依赖注入
- System.Configuration.ConfigurationManager - 配置管理

## 注意事项

1. 全局热键功能需要管理员权限才能正常工作
2. 崩溃转储功能在非Windows平台会生成文本格式的异常信息
3. 系统托盘功能需要Windows Forms支持
4. 插件会在应用程序启动时自动加载和初始化
5. 语言设置会保存在用户配置中，下次启动时自动应用

## 故障排除

- **热键不工作**: 检查是否有权限冲突，尝试以管理员身份运行
- **托盘图标不显示**: 检查Windows系统托盘设置
- **插件加载失败**: 查看日志文件中的详细错误信息
- **语言切换无效**: 确认资源文件存在且格式正确