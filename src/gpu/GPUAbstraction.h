#pragma once
#include <vector>
#include <string>
#include <memory>

namespace TCMT {
namespace GPU {

/**
 * GPU抽象接口 - 提供统一的GPU信息访问接口
 * 用于在C++核心和外部系统之间提供抽象层
 */
class IGPUProvider {
public:
    struct GPUInfo {
        std::wstring name;
        std::wstring brand;
        uint64_t memory = 0;
        double coreClock = 0.0;
        bool isVirtual = false;
        double temperature = 0.0;
        double utilization = 0.0;
    };

    virtual ~IGPUProvider() = default;
    
    /**
     * 获取所有GPU信息
     */
    virtual std::vector<GPUInfo> GetGPUList() = 0;
    
    /**
     * 获取主GPU信息
     */
    virtual GPUInfo GetPrimaryGPU() = 0;
    
    /**
     * 检查是否初始化成功
     */
    virtual bool IsInitialized() const = 0;
    
    /**
     * 刷新GPU信息
     */
    virtual void RefreshData() = 0;
};

/**
 * GPU管理器 - 管理GPU信息的访问和更新
 */
class GPUManager {
public:
    static GPUManager& GetInstance();
    
    /**
     * 设置GPU提供者
     */
    void SetProvider(std::unique_ptr<IGPUProvider> provider);
    
    /**
     * 获取所有GPU信息
     */
    std::vector<IGPUProvider::GPUInfo> GetGPUList();
    
    /**
     * 获取主GPU信息
     */
    IGPUProvider::GPUInfo GetPrimaryGPU();
    
    /**
     * 检查是否可用
     */
    bool IsAvailable() const;
    
    /**
     * 刷新数据
     */
    void RefreshData();

private:
    GPUManager() = default;
    std::unique_ptr<IGPUProvider> m_provider;
};

/**
 * 核心GPU提供者 - 使用现有的C++核心GPU实现
 */
class CoreGPUProvider : public IGPUProvider {
public:
    CoreGPUProvider();
    ~CoreGPUProvider() override;
    
    std::vector<GPUInfo> GetGPUList() override;
    GPUInfo GetPrimaryGPU() override;
    bool IsInitialized() const override;
    void RefreshData() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace GPU
} // namespace TCMT