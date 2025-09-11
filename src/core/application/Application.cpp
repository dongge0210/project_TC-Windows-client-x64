#include "Application.h"
#include "../Utils/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace TCMT {

Application& Application::GetInstance() {
    static Application instance;
    return instance;
}

Application::~Application() {
    Cleanup();
}

bool Application::Initialize(const std::string& configPath) {
    if (m_initialized) {
        return true;
    }
    
    Logger::Info("开始初始化TCMT应用程序");
    
    // 加载配置
    if (!LoadConfiguration(configPath)) {
        Logger::Error("加载配置失败");
        return false;
    }
    
    // 初始化GPU系统
    if (!InitializeGPU()) {
        Logger::Warning("GPU系统初始化失败，继续运行");
    }
    
    m_initialized = true;
    Logger::Info("TCMT应用程序初始化完成");
    return true;
}

bool Application::Start() {
    if (!m_initialized) {
        Logger::Error("应用程序未初始化");
        return false;
    }
    
    if (m_isRunning) {
        return true;
    }
    
    Logger::Info("启动TCMT应用程序");
    
    m_isRunning = true;
    Logger::Info("TCMT应用程序启动完成");
    return true;
}

void Application::Stop() {
    if (!m_isRunning) {
        return;
    }
    
    Logger::Info("停止TCMT应用程序");
    
    m_isRunning = false;
    Logger::Info("TCMT应用程序已停止");
}

void Application::Run() {
    if (!m_isRunning) {
        Logger::Error("应用程序未启动");
        return;
    }
    
    Logger::Info("进入主循环");
    
    // 主循环 - 可以根据需要实现
    while (m_isRunning) {
        // 处理消息、更新状态等
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    Logger::Info("退出主循环");
}

bool Application::IsRunning() const {
    return m_isRunning;
}

void Application::Cleanup() {
    Logger::Info("清理TCMT应用程序资源");
    
    Stop();
    
    m_initialized = false;
    Logger::Info("TCMT应用程序资源清理完成");
}

GPU::GPUManager& Application::GetGPUManager() {
    return GPU::GPUManager::GetInstance();
}

bool Application::InitializeGPU() {
    Logger::Info("初始化GPU系统");
    
    auto& gpuManager = GetGPUManager();
    
    // 设置核心GPU提供者
    auto coreProvider = std::make_unique<GPU::CoreGPUProvider>();
    bool success = coreProvider->IsInitialized();
    gpuManager.SetProvider(std::move(coreProvider));
    
    if (success) {
        Logger::Info("GPU系统初始化成功");
    } else {
        Logger::Warning("GPU系统初始化失败");
    }
    
    return success;
}

bool Application::LoadConfiguration(const std::string& configPath) {
    Logger::Info("加载配置文件");
    
    // 这里可以实现配置文件加载逻辑
    // 现在先返回true表示成功
    
    Logger::Info("配置文件加载完成");
    return true;
}

} // namespace TCMT