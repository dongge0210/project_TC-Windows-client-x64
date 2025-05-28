#pragma once
#include <vector>
#include <string>
#include <d3d11.h>

#if defined(SUPPORT_NVIDIA_GPU)
#ifdef USE_NVML
#include <nvml.h>
#endif
#endif

#if defined(SUPPORT_DIRECTX)
#include <dxgi.h>
#endif
#include <wbemidl.h>
#include "../DataStruct/DataStruct.h"
#include "../Utils/WmiManager.h"

// 确保没有与comutil.h冲突的类型、宏、using等
// 不要定义Data_t、operator=、operator+等与comutil.h同名的内容

// GPU基本信息结构体（不含温度和功率）
struct GPUDeviceInfo {
    std::string name;
    std::string vendor;
    std::string driverVersion;
    std::string deviceId;
    std::string status;
    std::string busId;
    bool isVirtual = false;
    bool isAvailable = false;
    // 温度和功率字段不在此结构体
};

class GpuInfo {
public:
    struct GpuData {
        std::wstring name;
        std::wstring deviceId;
        uint64_t vram = 0;          // 专用显存
        uint64_t sharedMemory = 0;  // 共享内存
        double coreClock = 0;
        int computeCapabilityMajor = 0;
        int computeCapabilityMinor = 0;
        std::wstring driverVersion;   // 保持为wstring
        std::wstring driverDate;      // 保持为wstring
        std::wstring driverProvider;  // 保持为wstring
        bool isNvidia = false;
        bool isAmd = false;
        bool isIntegrated = false;
        std::wstring brand;           // GPU品牌: "NVIDIA", "AMD", "Intel", 等
    };
    GpuInfo(WmiManager& manager);
    ~GpuInfo();
    void DetectGpusViaWmi();
    void QueryIntelGpuInfo(int index);
    void QueryNvidiaGpuInfo(int index);
    static double GetGpuPowerNVML();
    const std::vector<GpuData>& GetGpuData() const;

    // 获取所有物理GPU信息（不含温度和功率）
    static std::vector<GpuData> EnumPhysicalGPUs();

    // 判断是否为虚拟显卡
    static bool IsVirtualGPU(const std::string& name);

    // NVML初始化（如需主程序调用）
    static bool InitNVML();
    static bool nvmlInited;
    static bool IsNvmlEnabled() { return nvmlInited; } // 新增：提供NVML状态查询

private:
    WmiManager& wmiManager;
    IWbemServices* pSvc = nullptr;
    std::vector<GpuData> gpuList;
};
