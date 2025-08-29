#include "MemoryInfo.h"

#if PLATFORM_WINDOWS
    #include <windows.h>
#elif PLATFORM_LINUX
    #include <fstream>
    #include <sstream>
    #include <sys/sysinfo.h>
#elif PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <mach/vm_statistics.h>
    #include <mach/mach_host.h>
#endif

MemoryInfo::MemoryInfo() {
    UpdateMemoryStatus();
}

void MemoryInfo::UpdateMemoryStatus() {
#if PLATFORM_WINDOWS
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
#elif PLATFORM_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        totalPhysical = static_cast<MemorySize_t>(info.totalram) * info.mem_unit;
        availablePhysical = static_cast<MemorySize_t>(info.freeram) * info.mem_unit;
        totalVirtual = static_cast<MemorySize_t>(info.totalswap) * info.mem_unit + totalPhysical;
    } else {
        // Fallback: read from /proc/meminfo
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        totalPhysical = availablePhysical = totalVirtual = 0;
        
        while (std::getline(meminfo, line)) {
            std::istringstream iss(line);
            std::string key;
            MemorySize_t value;
            std::string unit;
            
            if (iss >> key >> value >> unit) {
                if (unit == "kB") value *= 1024; // Convert to bytes
                
                if (key == "MemTotal:") {
                    totalPhysical = value;
                } else if (key == "MemAvailable:") {
                    availablePhysical = value;
                } else if (key == "SwapTotal:") {
                    totalVirtual = totalPhysical + value;
                }
            }
        }
    }
#elif PLATFORM_MACOS
    size_t size = sizeof(MemorySize_t);
    
    // Get total physical memory
    if (sysctlbyname("hw.memsize", &totalPhysical, &size, nullptr, 0) != 0) {
        totalPhysical = 0;
    }
    
    // Get available memory using vm_statistics
    mach_port_t host_port = mach_host_self();
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);
    
    if (host_statistics64(host_port, HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size) == KERN_SUCCESS) {
        // Get page size
        vm_size_t page_size;
        host_page_size(host_port, &page_size);
        
        // Calculate available memory (free + inactive + purgeable + file-backed)
        availablePhysical = (vm_stat.free_count + vm_stat.inactive_count + 
                           vm_stat.purgeable_count + vm_stat.external_page_count) * page_size;
    } else {
        availablePhysical = 0;
    }
    
    // macOS doesn't have traditional swap in the same way, so we'll use physical memory as virtual
    totalVirtual = totalPhysical;
#endif
}

MemorySize_t MemoryInfo::GetTotalPhysical() const {
#if PLATFORM_WINDOWS
    return memStatus.ullTotalPhys;
#else
    return totalPhysical;
#endif
}

MemorySize_t MemoryInfo::GetAvailablePhysical() const {
#if PLATFORM_WINDOWS
    return memStatus.ullAvailPhys;
#else
    return availablePhysical;
#endif
}

MemorySize_t MemoryInfo::GetTotalVirtual() const {
#if PLATFORM_WINDOWS
    return memStatus.ullTotalVirtual;
#else
    return totalVirtual;
#endif
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           