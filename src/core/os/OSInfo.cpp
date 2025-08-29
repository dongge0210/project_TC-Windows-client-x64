#include "OSInfo.h"
#include "../Utils/PlatformUtils.h"

#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <winternl.h>
    #include <ntstatus.h>
    #ifndef STATUS_SUCCESS
    #define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
    #endif
    // 使用动态加载方式获取RtlGetVersion函数
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
#elif PLATFORM_LINUX
    #include <sys/utsname.h>
    #include <fstream>
#elif PLATFORM_MACOS
    #include <sys/utsname.h>
    #include <sys/sysctl.h>
#endif

OSInfo::OSInfo() {
#if PLATFORM_WINDOWS
    RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
    
    // 动态获取RtlGetVersion函数指针
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
        if (RtlGetVersion && RtlGetVersion(&osvi) == STATUS_SUCCESS) {
            majorVersion = osvi.dwMajorVersion;
            minorVersion = osvi.dwMinorVersion;
            buildNumber = osvi.dwBuildNumber;
            osVersion = "Windows " + 
                std::to_string(osvi.dwMajorVersion) + "." + 
                std::to_string(osvi.dwMinorVersion) + 
                " (Build " + std::to_string(osvi.dwBuildNumber) + ")";
        } else {
            osVersion = "Unknown Windows Version";
        }
    } else {
        osVersion = "Unknown Windows Version";
    }
#elif PLATFORM_LINUX
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        osVersion = std::string(unameData.sysname) + " " + 
                   std::string(unameData.release) + " " + 
                   std::string(unameData.machine);
        
        // Try to parse version numbers
        std::string release(unameData.release);
        size_t firstDot = release.find('.');
        size_t secondDot = release.find('.', firstDot + 1);
        
        if (firstDot != std::string::npos) {
            majorVersion = std::stoi(release.substr(0, firstDot));
            if (secondDot != std::string::npos) {
                minorVersion = std::stoi(release.substr(firstDot + 1, secondDot - firstDot - 1));
            }
        }
        
        // Try to read distribution info
        std::ifstream osRelease("/etc/os-release");
        if (osRelease.is_open()) {
            std::string line;
            while (std::getline(osRelease, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    size_t start = line.find('"');
                    size_t end = line.rfind('"');
                    if (start != std::string::npos && end != std::string::npos && start < end) {
                        osVersion = line.substr(start + 1, end - start - 1);
                        break;
                    }
                }
            }
            osRelease.close();
        }
    } else {
        osVersion = "Unknown Linux Version";
    }
#elif PLATFORM_MACOS
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        // Get macOS version using sysctl
        size_t size;
        sysctlbyname("kern.osproductversion", nullptr, &size, nullptr, 0);
        if (size > 0) {
            std::string version(size, '\0');
            if (sysctlbyname("kern.osproductversion", &version[0], &size, nullptr, 0) == 0) {
                version.resize(size - 1); // Remove null terminator
                osVersion = "macOS " + version + " (" + std::string(unameData.machine) + ")";
                
                // Parse version numbers
                size_t firstDot = version.find('.');
                size_t secondDot = version.find('.', firstDot + 1);
                
                if (firstDot != std::string::npos) {
                    majorVersion = std::stoi(version.substr(0, firstDot));
                    if (secondDot != std::string::npos) {
                        minorVersion = std::stoi(version.substr(firstDot + 1, secondDot - firstDot - 1));
                    }
                }
            }
        } else {
            osVersion = std::string("macOS ") + unameData.release + " (" + unameData.machine + ")";
        }
    } else {
        osVersion = "Unknown macOS Version";
    }
#endif
}

void OSInfo::Initialize() {
    // Initialization is done in constructor
    // This method is kept for compatibility
}

std::string OSInfo::GetVersion() const {
    return osVersion;
}

std::string OSInfo::GetVersion() const {
    return osVersion;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         