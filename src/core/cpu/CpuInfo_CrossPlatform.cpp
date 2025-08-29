#include "CpuInfo.h"
#include "../Utils/PlatformUtils.h"

#if PLATFORM_WINDOWS
    #include <intrin.h>
    #include <windows.h>
    #include <pdh.h>
    #include <algorithm>
    #pragma comment(lib, "pdh.lib")
#elif PLATFORM_LINUX
    #include <fstream>
    #include <sstream>
    #include <unistd.h>
    #include <thread>
#elif PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <sys/types.h>
    #include <mach/mach.h>
    #include <mach/processor_info.h>
    #include <mach/mach_host.h>
#endif

CpuInfo::CpuInfo() :
    totalCores(0),
    smallCores(0),
    largeCores(0),
    cpuUsage(0.0),
    lastUpdateTime(0),
    lastSampleTick(0),
    prevSampleTick(0),
    lastSampleIntervalMs(0.0),
    counterInitialized(false) {

#if !PLATFORM_WINDOWS
    prevIdleTime = 0;
    prevTotalTime = 0;
#endif

    try {
        DetectCores();
        cpuName = GetNameFromRegistry();
        InitializeCounter();
        UpdateCoreSpeeds();
    }
    catch (const std::exception& e) {
        // Simple error handling without Logger dependency
        cpuName = "Unknown CPU";
        totalCores = std::thread::hardware_concurrency();
        largeCores = totalCores;
        smallCores = 0;
    }
}

CpuInfo::~CpuInfo() {
    CleanupCounter();
}

void CpuInfo::DetectCores() {
#if PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    totalCores = sysInfo.dwNumberOfProcessors;

    DWORD bufferSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufferSize);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

    if (GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
        for (const auto& info : buffer) {
            if (info.Relationship == RelationProcessorCore) {
                (info.ProcessorCore.Flags == 1) ? largeCores++ : smallCores++;
            }
        }
    }
#elif PLATFORM_LINUX
    totalCores = std::thread::hardware_concurrency();
    // On Linux, we'll treat all cores as large cores for simplicity
    // More sophisticated detection would require parsing /proc/cpuinfo
    largeCores = totalCores;
    smallCores = 0;
    
    // Try to get more accurate core count from /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        int coreCount = 0;
        while (std::getline(cpuinfo, line)) {
            if (line.find("processor") == 0) {
                coreCount++;
            }
        }
        if (coreCount > 0) {
            totalCores = coreCount;
            largeCores = coreCount;
        }
        cpuinfo.close();
    }
#elif PLATFORM_MACOS
    size_t size = sizeof(int);
    if (sysctlbyname("hw.ncpu", &totalCores, &size, nullptr, 0) != 0) {
        totalCores = std::thread::hardware_concurrency();
    }
    
    // On macOS, treat all cores as large cores for simplicity
    largeCores = totalCores;
    smallCores = 0;
    
    // Try to get performance/efficiency core counts on Apple Silicon
    int perfCores = 0, effCores = 0;
    size = sizeof(int);
    if (sysctlbyname("hw.perflevel0.physicalcpu", &perfCores, &size, nullptr, 0) == 0) {
        size = sizeof(int);
        if (sysctlbyname("hw.perflevel1.physicalcpu", &effCores, &size, nullptr, 0) == 0) {
            largeCores = perfCores;
            smallCores = effCores;
            totalCores = perfCores + effCores;
        }
    }
#endif
}

std::string CpuInfo::GetNameFromRegistry() {
#if PLATFORM_WINDOWS
    HKEY hKey;
    char buffer[256];
    DWORD bufferSize = sizeof(buffer);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
    return "Unknown Windows CPU";
#elif PLATFORM_LINUX
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") == 0) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string name = line.substr(colonPos + 1);
                    // Trim whitespace
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_last_not_of(" \t") + 1);
                    cpuinfo.close();
                    return name;
                }
            }
        }
        cpuinfo.close();
    }
    return "Unknown Linux CPU";
#elif PLATFORM_MACOS
    size_t size;
    sysctlbyname("machdep.cpu.brand_string", nullptr, &size, nullptr, 0);
    if (size > 0) {
        std::string brand(size, '\0');
        if (sysctlbyname("machdep.cpu.brand_string", &brand[0], &size, nullptr, 0) == 0) {
            brand.resize(size - 1); // Remove null terminator
            return brand;
        }
    }
    return "Unknown macOS CPU";
#endif
}

void CpuInfo::InitializeCounter() {
#if PLATFORM_WINDOWS
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &queryHandle);
    if (status != ERROR_SUCCESS) {
        return;
    }

    status = PdhAddCounterA(queryHandle, "\\Processor(_Total)\\% Processor Time", 0, &counterHandle);
    if (status != ERROR_SUCCESS) {
        PdhCloseQuery(queryHandle);
        return;
    }

    // First call to collect initial data
    PdhCollectQueryData(queryHandle);
    counterInitialized = true;
#else
    // For Linux/macOS, we'll read CPU usage from system files
    counterInitialized = true;
    updateUsage(); // Initialize baseline values
#endif
}

void CpuInfo::CleanupCounter() {
#if PLATFORM_WINDOWS
    if (counterInitialized) {
        PdhCloseQuery(queryHandle);
        counterInitialized = false;
    }
#else
    counterInitialized = false;
#endif
}

double CpuInfo::updateUsage() {
#if PLATFORM_WINDOWS
    if (!counterInitialized) {
        return 0.0;
    }

    DWORD currentTick = GetTickCount();
    if (lastSampleTick > 0) {
        lastSampleIntervalMs = currentTick - lastSampleTick;
    }
    prevSampleTick = lastSampleTick;
    lastSampleTick = currentTick;

    PDH_STATUS status = PdhCollectQueryData(queryHandle);
    if (status != ERROR_SUCCESS) {
        return cpuUsage;
    }

    PDH_FMT_COUNTERVALUE counterValue;
    status = PdhGetFormattedCounterValue(counterHandle, PDH_FMT_DOUBLE, NULL, &counterValue);
    if (status == ERROR_SUCCESS && counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
        cpuUsage = counterValue.doubleValue;
    }

    return cpuUsage;
#elif PLATFORM_LINUX
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) {
        return cpuUsage;
    }

    std::string line;
    std::getline(stat, line);
    stat.close();

    if (line.substr(0, 3) == "cpu") {
        std::istringstream iss(line.substr(4));
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        uint64_t totalTime = user + nice + system + idle + iowait + irq + softirq + steal;
        uint64_t idleTime = idle + iowait;

        if (prevTotalTime > 0) {
            uint64_t totalDelta = totalTime - prevTotalTime;
            uint64_t idleDelta = idleTime - prevIdleTime;
            
            if (totalDelta > 0) {
                cpuUsage = ((double)(totalDelta - idleDelta) / totalDelta) * 100.0;
            }
        }

        prevTotalTime = totalTime;
        prevIdleTime = idleTime;
    }

    return cpuUsage;
#elif PLATFORM_MACOS
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        uint64_t totalTime = cpuinfo.cpu_ticks[CPU_STATE_USER] + 
                           cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] + 
                           cpuinfo.cpu_ticks[CPU_STATE_IDLE] + 
                           cpuinfo.cpu_ticks[CPU_STATE_NICE];
        uint64_t idleTime = cpuinfo.cpu_ticks[CPU_STATE_IDLE];

        if (prevTotalTime > 0) {
            uint64_t totalDelta = totalTime - prevTotalTime;
            uint64_t idleDelta = idleTime - prevIdleTime;
            
            if (totalDelta > 0) {
                cpuUsage = ((double)(totalDelta - idleDelta) / totalDelta) * 100.0;
            }
        }

        prevTotalTime = totalTime;
        prevIdleTime = idleTime;
    }

    return cpuUsage;
#endif
}

void CpuInfo::UpdateCoreSpeeds() {
#if PLATFORM_WINDOWS
    // Windows implementation would read from performance counters or registry
    // For now, we'll use a simple registry read
    HKEY hKey;
    DWORD speed = 0;
    DWORD size = sizeof(DWORD);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE)&speed, &size);
        RegCloseKey(hKey);
    }
    
    // Update speed vectors
    largeCoresSpeeds.assign(largeCores, speed);
    smallCoresSpeeds.assign(smallCores, speed);
    lastUpdateTime = GetTickCount();
#else
    // For Linux/macOS, we'll try to read CPU frequency information
    uint32_t baseSpeed = 2000; // Default 2GHz
    
#if PLATFORM_LINUX
    // Try to read from /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("cpu MHz") == 0) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string speedStr = line.substr(colonPos + 1);
                    baseSpeed = static_cast<uint32_t>(std::stod(speedStr));
                    break;
                }
            }
        }
        cpuinfo.close();
    }
#elif PLATFORM_MACOS
    // Try to read CPU frequency on macOS
    uint64_t freq;
    size_t size = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency", &freq, &size, nullptr, 0) == 0) {
        baseSpeed = static_cast<uint32_t>(freq / 1000000); // Convert Hz to MHz
    }
#endif
    
    largeCoresSpeeds.assign(largeCores, baseSpeed);
    smallCoresSpeeds.assign(smallCores, baseSpeed);
    
    // Use platform-specific time
    auto now = std::chrono::steady_clock::now();
    lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
#endif
}

// Public interface methods
double CpuInfo::GetUsage() {
    return updateUsage();
}

std::string CpuInfo::GetName() {
    return cpuName;
}

int CpuInfo::GetTotalCores() const {
    return totalCores;
}

int CpuInfo::GetSmallCores() const {
    return smallCores;
}

int CpuInfo::GetLargeCores() const {
    return largeCores;
}

double CpuInfo::GetLargeCoreSpeed() const {
    if (largeCoresSpeeds.empty()) return 0.0;
    return static_cast<double>(largeCoresSpeeds[0]);
}

double CpuInfo::GetSmallCoreSpeed() const {
    if (smallCoresSpeeds.empty()) return 0.0;
    return static_cast<double>(smallCoresSpeeds[0]);
}

CpuSpeed_t CpuInfo::GetCurrentSpeed() const {
    if (!largeCoresSpeeds.empty()) {
        return largeCoresSpeeds[0];
    }
    return 0;
}

bool CpuInfo::IsHyperThreadingEnabled() const {
    return (totalCores > (largeCores + smallCores));
}

bool CpuInfo::IsVirtualizationEnabled() const {
#if PLATFORM_WINDOWS
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    bool hasVMX = (cpuInfo[2] & (1 << 5)) != 0;

    if (!hasVMX) return false;

    bool isVMXEnabled = false;
    __try {
        unsigned __int64 msrValue = __readmsr(0x3A);
        isVMXEnabled = (msrValue & 0x5) != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        isVMXEnabled = false;
    }

    return isVMXEnabled;
#else
    // Simplified virtualization detection for Linux/macOS
    // This would require more sophisticated checking in a real implementation
    return false;
#endif
}