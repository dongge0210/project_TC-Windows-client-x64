#pragma once

// Platform detection macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_MACOS 0
    #define PLATFORM_LINUX 0
#elif defined(__APPLE__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_MACOS 1
    #define PLATFORM_LINUX 0
#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_MACOS 0
    #define PLATFORM_LINUX 1
#else
    #error "Unsupported platform"
#endif

// Platform-specific includes
#if PLATFORM_WINDOWS
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN  
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <sys/mount.h>
    #include <CoreFoundation/CoreFoundation.h>
    #include <IOKit/IOKitLib.h>
#elif PLATFORM_LINUX
    #include <sys/sysinfo.h>
    #include <sys/statfs.h>
    #include <unistd.h>
#endif

// Standard includes for all platforms
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>