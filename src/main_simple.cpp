// Cross-platform system monitor application
#include "core/platform/Platform.h"

// Standard library includes for all platforms
#include <iostream>
#include <chrono>
#include <thread>

// Project headers
#include "core/memory/MemoryInfo.h"
#include "core/os/OSInfo.h"
#include "core/Utils/PlatformUtils.h"

// Simple cross-platform system information display
void PrintSystemInfo() {
    std::cout << "=== System Information ===" << std::endl;
    
    // Platform info
    std::cout << "Platform: " << PlatformUtils::GetPlatformName() << std::endl;
    
    // OS info
    OSInfo osInfo;
    std::cout << "OS Version: " << osInfo.GetVersion() << std::endl;
    
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
        
        for (int i = 0; i < 10; ++i) {
            std::cout << "\n--- Update " << (i + 1) << " ---" << std::endl;
            
            MemoryInfo memInfo;
            std::cout << "Available Memory: " << (memInfo.GetAvailablePhysical() / (1024 * 1024)) << " MB" << std::endl;
            
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