#pragma once
#include "../platform/Platform.h"

#if PLATFORM_WINDOWS
#include <windows.h>
typedef ULONGLONG MemorySize_t;
#else
#include <cstdint>
typedef uint64_t MemorySize_t;
#endif

class MemoryInfo {
public:
    MemoryInfo();
    MemorySize_t GetTotalPhysical() const;
    MemorySize_t GetAvailablePhysical() const;
    MemorySize_t GetTotalVirtual() const;

private:
    void UpdateMemoryStatus();
    
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX memStatus;
#else
    MemorySize_t totalPhysical;
    MemorySize_t availablePhysical;  
    MemorySize_t totalVirtual;
#endif
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               