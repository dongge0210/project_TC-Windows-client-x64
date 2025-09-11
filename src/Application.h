#pragma once
#include "gpu/GPUAbstraction.h"
#include <string>
#include <memory>

namespace TCMT {

/**
 * TCMT应用程序核心 - 管理整个应用程序的生命周期
 * 简化版本，专注于硬件监控和GPU抽象层
 */
class Application {
public:
    static Application& GetInstance();
    
    /**
     * 初始化应用程序
     */
    bool Initialize(const std::string& configPath = "");
    
    /**
     * 启动应用程序
     */
    bool Start();
    
    /**
     * 停止应用程序
     */
    void Stop();
    
    /**
     * 运行应用程序主循环
     */
    void Run();
    
    /**
     * 检查应用程序是否在运行
     */
    bool IsRunning() const;
    
    /**
     * 清理应用程序资源
     */
    void Cleanup();
    
    /**
     * 获取GPU管理器
     */
    GPU::GPUManager& GetGPUManager();

private:
    Application() = default;
    ~Application();
    
    bool m_isRunning = false;
    bool m_initialized = false;
    
    /**
     * 初始化GPU系统
     */
    bool InitializeGPU();
    
    /**
     * 加载配置
     */
    bool LoadConfiguration(const std::string& configPath);
};

} // namespace TCMT