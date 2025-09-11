#pragma once
#include "ServiceInterfaces.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace TCMT {
namespace Services {

/**
 * 服务管理器 - 管理所有系统服务的生命周期
 */
class ServiceManager {
public:
    static ServiceManager& GetInstance();
    
    /**
     * 注册服务
     */
    template<typename T>
    bool RegisterService(const std::string& name, std::unique_ptr<T> service) {
        static_assert(std::is_base_of_v<ISystemService, T>, "T must inherit from ISystemService");
        
        if (m_services.find(name) != m_services.end()) {
            return false; // 服务已存在
        }
        
        if (!service->Initialize()) {
            return false; // 初始化失败
        }
        
        m_services[name] = std::move(service);
        return true;
    }
    
    /**
     * 获取服务
     */
    template<typename T>
    T* GetService(const std::string& name) {
        static_assert(std::is_base_of_v<ISystemService, T>, "T must inherit from ISystemService");
        
        auto it = m_services.find(name);
        if (it != m_services.end()) {
            return dynamic_cast<T*>(it->second.get());
        }
        return nullptr;
    }
    
    /**
     * 启动所有服务
     */
    bool StartAllServices();
    
    /**
     * 停止所有服务
     */
    void StopAllServices();
    
    /**
     * 启动特定服务
     */
    bool StartService(const std::string& name);
    
    /**
     * 停止特定服务
     */
    void StopService(const std::string& name);
    
    /**
     * 检查服务是否在运行
     */
    bool IsServiceRunning(const std::string& name) const;
    
    /**
     * 获取所有服务状态
     */
    std::unordered_map<std::string, bool> GetAllServiceStatus() const;
    
    /**
     * 清理所有服务
     */
    void Cleanup();

private:
    ServiceManager() = default;
    ~ServiceManager();
    
    std::unordered_map<std::string, std::unique_ptr<ISystemService>> m_services;
};

} // namespace Services
} // namespace TCMT