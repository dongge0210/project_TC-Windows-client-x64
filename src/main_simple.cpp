// Cross-platform system monitor application
#include "core/platform/Platform.h"

// Standard library includes for all platforms
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

// Project headers
#include "core/memory/MemoryInfo.h"
#include "core/os/OSInfo.h"
#include "core/Utils/PlatformUtils.h"
#include "core/cpu/CpuInfo.h"

// Simple cross-platform system information display
void PrintSystemInfo() {
    std::cout << "=== System Information ===" << std::endl;
    
    // Platform info
    std::cout << "Platform: " << PlatformUtils::GetPlatformName() << std::endl;
    
    // OS info
    OSInfo osInfo;
    std::cout << "OS Version: " << osInfo.GetVersion() << std::endl;
    
    // CPU info
    CpuInfo cpuInfo;
    std::cout << "CPU: " << cpuInfo.GetName() << std::endl;
    std::cout << "Total Cores: " << cpuInfo.GetTotalCores() << std::endl;
    std::cout << "Large Cores: " << cpuInfo.GetLargeCores() << std::endl;
    std::cout << "Small Cores: " << cpuInfo.GetSmallCores() << std::endl;
    std::cout << "CPU Speed: " << cpuInfo.GetCurrentSpeed() << " MHz" << std::endl;
    std::cout << "Hyper-Threading: " << (cpuInfo.IsHyperThreadingEnabled() ? "Enabled" : "Disabled") << std::endl;
    
    // Memory info
    MemoryInfo memInfo;
    std::cout << "Total Physical Memory: " << (memInfo.GetTotalPhysical() / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "Available Physical Memory: " << (memInfo.GetAvailablePhysical() / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "Total Virtual Memory: " << (memInfo.GetTotalVirtual() / (1024 * 1024 * 1024)) << " GB" << std::endl;
    
    std::cout << "=========================" << std::endl;
}

// Cross-platform main function
int main(int argc, char* argv[]) {
    std::cout << "Cross-Platform System Monitor" << std::endl;
    std::cout << "Compiled for: " << PlatformUtils::GetPlatformName() << std::endl;
    std::cout << std::endl;
    
    try {
        // Display basic system information
        PrintSystemInfo();
        
        // Simple monitoring loop
        std::cout << std::endl << "Monitoring system (press Ctrl+C to exit)..." << std::endl;
        
        CpuInfo cpuInfo;  // Create CPU info for monitoring
        
        for (int i = 0; i < 10; ++i) {
            std::cout << "\n--- Update " << (i + 1) << " ---" << std::endl;
            
            MemoryInfo memInfo;
            std::cout << "Available Memory: " << (memInfo.GetAvailablePhysical() / (1024 * 1024)) << " MB" << std::endl;
            std::cout << "CPU Usage: " << std::fixed << std::setprecision(1) << cpuInfo.GetUsage() << "%" << std::endl;
            
            // Wait for 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        std::cout << std::endl << "Monitoring complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}