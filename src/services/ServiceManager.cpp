#include "ServiceManager.h"

namespace TCMT {
namespace Services {

ServiceManager& ServiceManager::GetInstance() {
    static ServiceManager instance;
    return instance;
}

ServiceManager::~ServiceManager() {
    Cleanup();
}

bool ServiceManager::StartAllServices() {
    bool allStarted = true;
    
    for (auto& [name, service] : m_services) {
        if (!service->Start()) {
            allStarted = false;
            // 可以添加日志记录
        }
    }
    
    return allStarted;
}

void ServiceManager::StopAllServices() {
    for (auto& [name, service] : m_services) {
        if (service->IsRunning()) {
            service->Stop();
        }
    }
}

bool ServiceManager::StartService(const std::string& name) {
    auto it = m_services.find(name);
    if (it != m_services.end()) {
        return it->second->Start();
    }
    return false;
}

void ServiceManager::StopService(const std::string& name) {
    auto it = m_services.find(name);
    if (it != m_services.end() && it->second->IsRunning()) {
        it->second->Stop();
    }
}

bool ServiceManager::IsServiceRunning(const std::string& name) const {
    auto it = m_services.find(name);
    if (it != m_services.end()) {
        return it->second->IsRunning();
    }
    return false;
}

std::unordered_map<std::string, bool> ServiceManager::GetAllServiceStatus() const {
    std::unordered_map<std::string, bool> status;
    
    for (const auto& [name, service] : m_services) {
        status[name] = service->IsRunning();
    }
    
    return status;
}

void ServiceManager::Cleanup() {
    StopAllServices();
    m_services.clear();
}

} // namespace Services
} // namespace TCMT