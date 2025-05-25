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

class GpuInfo {
public:
    struct GpuData {
        std::wstring name;
        std::wstring deviceId;
        std::wstring brand;
        uint64_t vram = 0;          // 专用显存
        uint64_t sharedMemory = 0;  // 共享内存
        double coreClock = 0;
        bool isNvidia = false;
        bool isAmd = false;
        bool isIntegrated = false;
        int computeCapabilityMajor = 0;
        int computeCapabilityMinor = 0;
        std::wstring driverVersion;   // 保持为wstring
        std::wstring driverDate;      // 保持为wstring
        std::wstring driverProvider;  // 保持为wstring
    };
    GpuInfo(WmiManager& manager);
    ~GpuInfo();
    void DetectGpusViaWmi();
    void QueryIntelGpuInfo(int index);
    void QueryNvidiaGpuInfo(int index);
    static double GetGpuPowerNVML();
    const std::vector<GpuData>& GetGpuData() const;

private:
    WmiManager& wmiManager;
    IWbemServices* pSvc = nullptr;
    std::vector<GpuData> gpuList;
};
