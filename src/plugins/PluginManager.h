#pragma once
#include <string>
#include <vector>
#include <memory>

namespace TCMT {
namespace Plugins {

/**
 * 插件接口 - 所有插件必须实现此接口
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * 获取插件名称
     */
    virtual std::string GetName() const = 0;
    
    /**
     * 获取插件版本
     */
    virtual std::string GetVersion() const = 0;
    
    /**
     * 获取插件描述
     */
    virtual std::string GetDescription() const = 0;
    
    /**
     * 获取插件作者
     */
    virtual std::string GetAuthor() const = 0;
    
    /**
     * 初始化插件
     */
    virtual bool Initialize() = 0;
    
    /**
     * 启动插件
     */
    virtual bool Start() = 0;
    
    /**
     * 停止插件
     */
    virtual void Stop() = 0;
    
    /**
     * 清理插件资源
     */
    virtual void Cleanup() = 0;
    
    /**
     * 检查插件是否在运行
     */
    virtual bool IsRunning() const = 0;
    
    /**
     * 获取插件配置接口（可选）
     */
    virtual void* GetConfigInterface() { return nullptr; }
};

/**
 * 插件信息结构
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string filePath;
    bool isLoaded = false;
    bool isRunning = false;
};

/**
 * 插件管理器 - 管理插件的加载、卸载和执行
 */
class PluginManager {
public:
    static PluginManager& GetInstance();
    
    /**
     * 扫描插件目录
     */
    void ScanPluginDirectory(const std::string& directory);
    
    /**
     * 加载插件
     */
    bool LoadPlugin(const std::string& pluginPath);
    
    /**
     * 卸载插件
     */
    bool UnloadPlugin(const std::string& pluginName);
    
    /**
     * 启动插件
     */
    bool StartPlugin(const std::string& pluginName);
    
    /**
     * 停止插件
     */
    bool StopPlugin(const std::string& pluginName);
    
    /**
     * 获取所有插件信息
     */
    std::vector<PluginInfo> GetAllPlugins() const;
    
    /**
     * 获取运行中的插件
     */
    std::vector<PluginInfo> GetRunningPlugins() const;
    
    /**
     * 获取特定插件
     */
    IPlugin* GetPlugin(const std::string& pluginName);
    
    /**
     * 启动所有插件
     */
    void StartAllPlugins();
    
    /**
     * 停止所有插件
     */
    void StopAllPlugins();
    
    /**
     * 清理所有插件
     */
    void Cleanup();

private:
    PluginManager() = default;
    ~PluginManager();
    
    struct PluginEntry {
        std::unique_ptr<IPlugin> plugin;
        PluginInfo info;
        void* handle = nullptr;  // DLL句柄
    };
    
    std::vector<PluginEntry> m_plugins;
    
    /**
     * 从DLL加载插件
     */
    std::unique_ptr<IPlugin> LoadPluginFromDLL(const std::string& dllPath);
    
    /**
     * 卸载DLL
     */
    void UnloadDLL(void* handle);
    
    /**
     * 验证插件
     */
    bool ValidatePlugin(IPlugin* plugin);
};

} // namespace Plugins
} // namespace TCMT