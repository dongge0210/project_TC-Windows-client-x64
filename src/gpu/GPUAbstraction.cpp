#include "GPUAbstraction.h"
#include "../core/gpu/GpuInfo.h"
#include "../core/Utils/WmiManager.h"
#include <memory>

namespace TCMT {
namespace GPU {

// CoreGPUProvider实现
class CoreGPUProvider::Impl {
public:
    Impl() {
        try {
            // 初始化WMI管理器
            wmiManager = std::make_unique<WmiManager>();
            if (wmiManager->Initialize()) {
                // 初始化GPU信息收集器
                gpuInfo = std::make_unique<GpuInfo>(*wmiManager);
                initialized = true;
            }
        }
        catch (const std::exception& e) {
            // 日志记录初始化失败
            initialized = false;
        }
    }
    
    ~Impl() = default;
    
    std::vector<IGPUProvider::GPUInfo> GetGPUList() {
        std::vector<IGPUProvider::GPUInfo> result;
        
        if (!initialized || !gpuInfo) {
            return result;
        }
        
        try {
            const auto& coreGpuData = gpuInfo->GetGpuData();
            
            for (const auto& gpu : coreGpuData) {
                IGPUProvider::GPUInfo info;
                info.name = gpu.name;
                info.brand = ExtractBrand(gpu.name);
                info.memory = gpu.dedicatedMemory;
                info.coreClock = gpu.coreClock;
                info.isVirtual = gpu.isVirtual;
                info.temperature = static_cast<double>(gpu.temperature);
                info.utilization = 0.0; // 需要额外实现
                
                result.push_back(info);
            }
        }
        catch (const std::exception& e) {
            // 日志记录错误
        }
        
        return result;
    }
    
    IGPUProvider::GPUInfo GetPrimaryGPU() {
        auto gpuList = GetGPUList();
        if (!gpuList.empty()) {
            return gpuList[0];
        }
        
        // 返回空的GPU信息
        IGPUProvider::GPUInfo empty;
        empty.name = L"未检测到GPU";
        return empty;
    }
    
    bool IsInitialized() const {
        return initialized;
    }
    
    void RefreshData() {
        if (initialized && gpuInfo) {
            // 重新检测GPU信息（如果核心支持）
            // 目前核心实现在构造时检测，可能需要扩展
        }
    }

private:
    std::wstring ExtractBrand(const std::wstring& name) {
        std::wstring lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        if (lowerName.find(L"nvidia") != std::wstring::npos || 
            lowerName.find(L"geforce") != std::wstring::npos ||
            lowerName.find(L"quadro") != std::wstring::npos) {
            return L"NVIDIA";
        }
        else if (lowerName.find(L"amd") != std::wstring::npos || 
                 lowerName.find(L"radeon") != std::wstring::npos) {
            return L"AMD";
        }
        else if (lowerName.find(L"intel") != std::wstring::npos) {
            return L"Intel";
        }
        
        return L"Unknown";
    }
    
    std::unique_ptr<WmiManager> wmiManager;
    std::unique_ptr<GpuInfo> gpuInfo;
    bool initialized = false;
};

// CoreGPUProvider实现
CoreGPUProvider::CoreGPUProvider() : m_impl(std::make_unique<Impl>()) {}

CoreGPUProvider::~CoreGPUProvider() = default;

std::vector<IGPUProvider::GPUInfo> CoreGPUProvider::GetGPUList() {
    return m_impl->GetGPUList();
}

IGPUProvider::GPUInfo CoreGPUProvider::GetPrimaryGPU() {
    return m_impl->GetPrimaryGPU();
}

bool CoreGPUProvider::IsInitialized() const {
    return m_impl->IsInitialized();
}

void CoreGPUProvider::RefreshData() {
    m_impl->RefreshData();
}

// GPUManager实现
GPUManager& GPUManager::GetInstance() {
    static GPUManager instance;
    return instance;
}

void GPUManager::SetProvider(std::unique_ptr<IGPUProvider> provider) {
    m_provider = std::move(provider);
}

std::vector<IGPUProvider::GPUInfo> GPUManager::GetGPUList() {
    if (m_provider) {
        return m_provider->GetGPUList();
    }
    return {};
}

IGPUProvider::GPUInfo GPUManager::GetPrimaryGPU() {
    if (m_provider) {
        return m_provider->GetPrimaryGPU();
    }
    
    IGPUProvider::GPUInfo empty;
    empty.name = L"未初始化";
    return empty;
}

bool GPUManager::IsAvailable() const {
    return m_provider && m_provider->IsInitialized();
}

void GPUManager::RefreshData() {
    if (m_provider) {
        m_provider->RefreshData();
    }
}

} // namespace GPU
} // namespace TCMT