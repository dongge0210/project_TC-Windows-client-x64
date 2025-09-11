# TCMT Windows Client 架构文档

## 项目结构

```
TCMT-Windows-client-x64/
├── src/                           # C++ 核心代码
│   ├── core/                      # 核心硬件监控模块
│   │   ├── cpu/                   # CPU 信息收集
│   │   ├── gpu/                   # GPU 信息收集
│   │   ├── memory/                # 内存信息收集
│   │   ├── disk/                  # 磁盘信息收集
│   │   ├── network/               # 网络信息收集
│   │   ├── temperature/           # 温度监控
│   │   ├── tpm/                   # TPM 信息收集
│   │   ├── os/                    # 操作系统信息
│   │   ├── DataStruct/            # 数据结构定义
│   │   └── Utils/                 # 工具类
│   ├── gpu/                       # GPU 抽象层 (新增)
│   │   ├── GPUAbstraction.h       # GPU 抽象接口
│   │   └── GPUAbstraction.cpp     # GPU 抽象实现
│   ├── services/                  # 系统服务 (新增)
│   │   ├── ServiceInterfaces.h    # 服务接口定义
│   │   ├── ServiceManager.h       # 服务管理器头文件
│   │   └── ServiceManager.cpp     # 服务管理器实现
│   ├── plugins/                   # 插件系统 (新增)
│   │   └── PluginManager.h        # 插件管理器
│   ├── Application.h              # 应用程序主类头文件
│   ├── Application.cpp            # 应用程序主类实现
│   ├── main.cpp                   # C++ 主入口点
│   └── third_party/               # 第三方库
├── WPF-UI1/                       # WPF 显示层 (精简)
│   ├── Services/                  # 显示相关服务
│   │   ├── LocalizationService.cs # 国际化服务
│   │   └── SharedMemoryService.cs # 共享内存通信
│   ├── Models/                    # 数据模型
│   │   └── SystemInfo.cs          # 系统信息模型
│   ├── ViewModels/                # 视图模型
│   │   └── MainWindowViewModel.cs # 主窗口视图模型
│   ├── Converters/                # 值转换器
│   │   └── ValueConverters.cs     # 数据转换器
│   ├── Resources/                 # 资源文件
│   │   └── Localization/          # 国际化资源
│   │       ├── Strings.resx       # 简体中文 (默认)
│   │       ├── Strings.zh-TW.resx # 繁体中文
│   │       └── Strings.en-US.resx # 英语
│   ├── Properties/                # 应用程序属性
│   ├── MainWindow.xaml            # 主窗口界面
│   ├── MainWindow.xaml.cs         # 主窗口代码
│   ├── App.xaml                   # 应用程序资源
│   └── App.xaml.cs                # 应用程序入口
└── Project1/                      # 原有项目 (保持不变)
```

## 架构原则

### 1. 分离关注点
- **C++ 核心 (src/)**: 负责硬件数据收集、系统服务、插件管理
- **WPF UI (WPF-UI1/)**: 专注于数据显示和用户交互

### 2. 清晰的数据流
```
硬件 → C++ 核心收集 → 共享内存 → WPF UI 显示
```

### 3. 模块化设计
- 每个功能模块独立开发和测试
- 通过接口实现松耦合
- 支持插件扩展

## 核心组件

### C++ 核心层

#### 1. Application 类
- 应用程序主控制器
- 管理所有子系统的生命周期
- 提供统一的访问入口

#### 2. GPU 抽象层 (src/gpu/)
- 提供统一的 GPU 信息访问接口
- 封装现有的 C++ GPU 实现
- 支持多种 GPU 提供者

#### 3. 服务管理系统 (src/services/)
- 管理系统级服务 (热键、托盘、监控等)
- 提供服务注册和生命周期管理
- 支持服务间依赖关系

#### 4. 插件系统 (src/plugins/)
- 支持动态加载 DLL 插件
- 提供插件生命周期管理
- 标准化的插件接口

### WPF 显示层

#### 1. 共享内存服务
- 从 C++ 核心读取硬件数据
- 提供类型安全的数据访问
- 处理数据结构映射

#### 2. 国际化服务
- 支持 3 种语言切换
- 动态语言更新
- 用户设置持久化

#### 3. MVVM 架构
- 视图模型负责数据绑定
- 转换器处理数据格式化
- 清晰的视图逻辑分离

## 数据通信

### 共享内存结构
```cpp
struct SharedMemoryBlock {
    // CPU 信息
    wchar_t cpuName[128];
    int physicalCores;
    int logicalCores;
    double cpuUsage;
    
    // GPU 信息
    GPUDataStruct gpus[2];
    int gpuCount;
    
    // 内存信息
    uint64_t totalMemory;
    uint64_t usedMemory;
    uint64_t availableMemory;
    
    // 其他硬件信息...
    SYSTEMTIME lastUpdate;
};
```

### 数据更新流程
1. C++ 核心定期收集硬件数据
2. 数据写入共享内存块
3. WPF UI 读取共享内存数据
4. 通过数据绑定更新界面

## 扩展性

### 1. 新增硬件支持
- 在 `src/core/` 添加新的硬件模块
- 更新共享内存结构
- 在 WPF 中添加对应的显示模型

### 2. 新增服务
- 实现 `ISystemService` 接口
- 通过 `ServiceManager` 注册服务
- 在 `Application` 中初始化

### 3. 新增插件
- 实现 `IPlugin` 接口
- 编译为 DLL 文件
- 放置在 plugins/ 目录

## 构建与部署

### 依赖项
- C++17 或更高版本
- .NET Framework 4.8 或 .NET Core 3.1+
- Windows SDK
- DirectX SDK (用于 GPU 检测)

### 构建步骤
1. 构建 C++ 核心项目
2. 构建 WPF UI 项目
3. 复制必要的 DLL 和资源文件
4. 打包为可分发版本

这个架构确保了 C++ 核心负责所有系统级操作，而 WPF UI 专注于用户界面显示，实现了清晰的职责分离。