#include "GPUAbstraction.h"
#include "../core/gpu/GpuInfo.h"
#include "../core/Utils/WmiManager.h"
#include <memory>
#include <algorithm>
#include <cctype>

// 预留API支持检测的头文件包含
#ifdef SUPPORT_CUDA
// #include <cuda_runtime.h>
// #include <nvml.h>
#endif

#ifdef SUPPORT_VULKAN
// #include <vulkan/vulkan.h>
#endif

#ifdef SUPPORT_OPENGL
// #include <GL/gl.h>
#endif

#ifdef SUPPORT_D3D
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#endif

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
                
                // 检测支持的图形API
                info.supportedAPIs = DetectSupportedAPIs(gpu.name, info.brand);
                
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
    
    bool SupportsAPI(const std::wstring& gpuName, GraphicsAPI api) {
        if (!initialized || !gpuInfo) {
            return false;
        }
        
        switch (api) {
            case GraphicsAPI::CUDA_12_6_PLUS:
                return CheckCUDASupport(gpuName);
            case GraphicsAPI::VULKAN:
                return CheckVulkanSupport(gpuName);
            case GraphicsAPI::OPENGL_4_1_PLUS:
                return CheckOpenGLSupport(gpuName);
            case GraphicsAPI::D3D12:
                return CheckD3D12Support(gpuName);
            case GraphicsAPI::D3D11:
                return CheckD3D11Support(gpuName);
            default:
                return false;
        }
    }
    
    void RefreshData() {
        if (initialized && gpuInfo) {
            // 重新检测GPU信息（如果核心支持）
            // 目前核心实现在构造时检测，可能需要扩展
        }
    }

private:
    std::vector<GraphicsAPI> DetectSupportedAPIs(const std::wstring& gpuName, const std::wstring& brand) {
        std::vector<GraphicsAPI> apis;
        
        // 检测各种API支持
        if (CheckCUDASupport(gpuName)) {
            apis.push_back(GraphicsAPI::CUDA_12_6_PLUS);
        }
        if (CheckVulkanSupport(gpuName)) {
            apis.push_back(GraphicsAPI::VULKAN);
        }
        if (CheckOpenGLSupport(gpuName)) {
            apis.push_back(GraphicsAPI::OPENGL_4_1_PLUS);
        }
        if (CheckD3D12Support(gpuName)) {
            apis.push_back(GraphicsAPI::D3D12);
        }
        if (CheckD3D11Support(gpuName)) {
            apis.push_back(GraphicsAPI::D3D11);
        }
        
        return apis;
    }
    
    bool CheckCUDASupport(const std::wstring& gpuName) {
        // CUDA 12.6+ 支持检测 - 预留实现
        std::wstring lowerName = gpuName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        // NVIDIA GPU通常支持CUDA，但需要进一步验证版本
        if (lowerName.find(L"nvidia") != std::wstring::npos || 
            lowerName.find(L"geforce") != std::wstring::npos ||
            lowerName.find(L"quadro") != std::wstring::npos ||
            lowerName.find(L"tesla") != std::wstring::npos) {
            // 这里可以添加CUDA版本检测逻辑
            // 目前预留为基础支持检测
            return true;
        }
        return false;
    }
    
    bool CheckVulkanSupport(const std::wstring& gpuName) {
        // Vulkan支持检测 - 预留实现
        std::wstring lowerName = gpuName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        // 大多数现代GPU都支持Vulkan
        if (lowerName.find(L"nvidia") != std::wstring::npos || 
            lowerName.find(L"amd") != std::wstring::npos ||
            lowerName.find(L"radeon") != std::wstring::npos ||
            lowerName.find(L"intel") != std::wstring::npos) {
            // 这里可以添加具体的Vulkan实例创建测试
            return true;
        }
        return false;
    }
    
    bool CheckOpenGLSupport(const std::wstring& gpuName) {
        // OpenGL 4.1+ 支持检测 - 预留实现
        std::wstring lowerName = gpuName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        // 大多数现代GPU都支持OpenGL 4.1+
        if (lowerName.find(L"nvidia") != std::wstring::npos || 
            lowerName.find(L"amd") != std::wstring::npos ||
            lowerName.find(L"radeon") != std::wstring::npos ||
            lowerName.find(L"intel") != std::wstring::npos) {
            // 这里可以添加OpenGL上下文创建和版本检测
            return true;
        }
        return false;
    }
    
    bool CheckD3D12Support(const std::wstring& gpuName) {
        // DirectX 12支持检测 - 预留实现
        try {
            // 可以尝试创建D3D12设备进行检测
            // 目前基于GPU类型进行基础判断
            std::wstring lowerName = gpuName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
            
            // Windows 10+ 的现代GPU通常支持D3D12
            if (lowerName.find(L"nvidia") != std::wstring::npos || 
                lowerName.find(L"amd") != std::wstring::npos ||
                lowerName.find(L"radeon") != std::wstring::npos) {
                return true;
            }
            // Intel集成显卡的D3D12支持需要更细致的检测
            if (lowerName.find(L"intel") != std::wstring::npos) {
                return true; // 大部分现代Intel GPU支持D3D12
            }
        }
        catch (...) {
            return false;
        }
        return false;
    }
    
    bool CheckD3D11Support(const std::wstring& gpuName) {
        // DirectX 11支持检测 - 预留实现
        try {
            // D3D11支持更广泛，大多数GPU都支持
            std::wstring lowerName = gpuName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
            
            if (lowerName.find(L"nvidia") != std::wstring::npos || 
                lowerName.find(L"amd") != std::wstring::npos ||
                lowerName.find(L"radeon") != std::wstring::npos ||
                lowerName.find(L"intel") != std::wstring::npos) {
                return true;
            }
        }
        catch (...) {
            return false;
        }
        return false;
    }
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

bool CoreGPUProvider::SupportsAPI(const std::wstring& gpuName, GraphicsAPI api) {
    return m_impl->SupportsAPI(gpuName, api);
}

void CoreGPUProvider::RefreshData() {
    m_impl->RefreshData();
}

bool GPUManager::CheckAPISupport(const std::wstring& gpuName, GraphicsAPI api) {
    if (m_provider) {
        return m_provider->SupportsAPI(gpuName, api);
    }
    return false;
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