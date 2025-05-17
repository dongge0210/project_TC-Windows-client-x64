#pragma once
#include <windows.h>
#include <cstdint>

class MemoryInfo {
private:
    MEMORYSTATUSEX memStatus;
    uint32_t memoryFrequency;

    // 新增的私有方法
    uint32_t DetectMemoryFrequencyMHz();
    uint32_t GetMemoryFrequencyViaWMI();
    uint32_t EstimateMemoryFrequencyFromCPU();

public:
    MemoryInfo();
    ULONGLONG GetTotalPhysical() const;
    ULONGLONG GetAvailablePhysical() const;
    ULONGLONG GetTotalVirtual() const;
    uint32_t GetMemoryFrequency() const;
};