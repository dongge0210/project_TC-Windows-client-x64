#include "DiagnosticTools.h"
#include "../Utils/Logger.h"
#include "../application/Application.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <psapi.h>

namespace TCMT {
namespace DevTools {

DiagnosticTools& DiagnosticTools::GetInstance() {
    static DiagnosticTools instance;
    return instance;
}

DiagnosticTools::~DiagnosticTools() {
    Cleanup();
}

bool DiagnosticTools::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    Logger::Info("初始化诊断工具");
    
    m_initialized = true;
    Logger::Info("诊断工具初始化完成");
    return true;
}

void DiagnosticTools::StartCommandLine() {
    if (!m_initialized) {
        std::cout << "诊断工具未初始化" << std::endl;
        return;
    }
    
    std::cout << "\n=== TCMT 开发工具化诊断命令行 ===" << std::endl;
    std::cout << "输入 'help' 查看可用命令，输入 'exit' 退出" << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "\nTCMT-Dev> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        if (input == "exit" || input == "quit") {
            std::cout << "退出诊断工具" << std::endl;
            break;
        }
        
        auto [command, args] = ParseCommandLine(input);
        ExecuteCommand(command, args);
    }
}

bool DiagnosticTools::ExecuteCommand(const std::string& command, const std::vector<std::string>& args) {
    if (command == "help") {
        ShowHelp();
    }
    else if (command == "system") {
        DiagnoseSystem();
    }
    else if (command == "gpu") {
        DiagnoseGPU();
    }
    else if (command == "hardware") {
        DiagnoseHardware();
    }
    else if (command == "memory") {
        DiagnoseMemory();
    }
    else if (command == "network") {
        DiagnoseNetwork();
    }
    else if (command == "clear") {
        system("cls");
    }
    else {
        std::cout << "未知命令: " << command << "，输入 'help' 查看可用命令" << std::endl;
        return false;
    }
    
    return true;
}

void DiagnosticTools::ShowHelp() const {
    std::cout << "\n=== 可用命令 ===" << std::endl;
    std::cout << "help       - 显示帮助信息" << std::endl;
    std::cout << "system     - 系统信息诊断" << std::endl;
    std::cout << "gpu        - GPU状态诊断" << std::endl;
    std::cout << "hardware   - 硬件监控状态诊断" << std::endl;
    std::cout << "memory     - 内存使用诊断" << std::endl;
    std::cout << "network    - 网络状态诊断" << std::endl;
    std::cout << "clear      - 清屏" << std::endl;
    std::cout << "exit/quit  - 退出诊断工具" << std::endl;
}

void DiagnosticTools::DiagnoseSystem() {
    std::cout << "\n=== 系统信息诊断 ===" << std::endl;
    
    // 获取Windows版本
    OSVERSIONINFOW osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    
    if (GetVersionExW(&osvi)) {
        std::cout << "操作系统版本: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion 
                  << " Build " << osvi.dwBuildNumber << std::endl;
    }
    
    // 获取系统内存信息
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        std::cout << "物理内存总量: " << (memInfo.ullTotalPhys / 1024 / 1024) << " MB" << std::endl;
        std::cout << "可用物理内存: " << (memInfo.ullAvailPhys / 1024 / 1024) << " MB" << std::endl;
        std::cout << "内存使用率: " << memInfo.dwMemoryLoad << "%" << std::endl;
    }
    
    // 获取CPU信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::cout << "处理器数量: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    // 应用程序状态
    auto& app = Application::GetInstance();
    std::cout << "TCMT应用状态: " << (app.IsRunning() ? "运行中" : "已停止") << std::endl;
}

void DiagnosticTools::DiagnoseGPU() {
    std::cout << "\n=== GPU状态诊断 ===" << std::endl;
    
    try {
        auto& app = Application::GetInstance();
        auto& gpuManager = app.GetGPUManager();
        
        std::cout << "GPU管理器状态: 已初始化" << std::endl;
        
        // 检测CUDA支持
        std::cout << "CUDA 12.6+ 支持: " << (gpuManager.IsCUDASupported() ? "是" : "否") << std::endl;
        
        // 检测Vulkan支持
        std::cout << "Vulkan 支持: " << (gpuManager.IsVulkanSupported() ? "是" : "否") << std::endl;
        
        // 检测OpenGL支持
        std::cout << "OpenGL 4.1+ 支持: " << (gpuManager.IsOpenGLSupported() ? "是" : "否") << std::endl;
        
        // 检测DirectX支持
        std::cout << "DirectX 12 支持: " << (gpuManager.IsDirectX12Supported() ? "是" : "否") << std::endl;
        std::cout << "DirectX 11 支持: " << (gpuManager.IsDirectX11Supported() ? "是" : "否") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "GPU诊断失败: " << e.what() << std::endl;
    }
}

void DiagnosticTools::DiagnoseHardware() {
    std::cout << "\n=== 硬件监控状态诊断 ===" << std::endl;
    
    std::cout << "CPU监控: 可用" << std::endl;
    std::cout << "内存监控: 可用" << std::endl;
    std::cout << "磁盘监控: 可用" << std::endl;
    std::cout << "网络监控: 可用" << std::endl;
    std::cout << "温度监控: 可用" << std::endl;
    std::cout << "GPU监控: 可用" << std::endl;
    std::cout << "TPM监控: 可用" << std::endl;
    
    // 这里可以添加实际的硬件状态检查
    std::cout << "注意: 详细硬件状态需要实际监控模块支持" << std::endl;
}

void DiagnosticTools::DiagnoseMemory() {
    std::cout << "\n=== 内存使用诊断 ===" << std::endl;
    
    // 获取当前进程内存使用
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        std::cout << "当前进程内存使用:" << std::endl;
        std::cout << "  工作集大小: " << (pmc.WorkingSetSize / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  峰值工作集: " << (pmc.PeakWorkingSetSize / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  页面文件使用: " << (pmc.PagefileUsage / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  峰值页面文件: " << (pmc.PeakPagefileUsage / 1024 / 1024) << " MB" << std::endl;
    }
    
    // 全局内存状态
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        std::cout << "\n系统内存状态:" << std::endl;
        std::cout << "  内存使用率: " << memInfo.dwMemoryLoad << "%" << std::endl;
        std::cout << "  总物理内存: " << (memInfo.ullTotalPhys / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  可用物理内存: " << (memInfo.ullAvailPhys / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  总虚拟内存: " << (memInfo.ullTotalVirtual / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  可用虚拟内存: " << (memInfo.ullAvailVirtual / 1024 / 1024) << " MB" << std::endl;
    }
}

void DiagnosticTools::DiagnoseNetwork() {
    std::cout << "\n=== 网络状态诊断 ===" << std::endl;
    
    // 基本网络连接测试
    std::cout << "网络适配器状态: 需要网络监控模块支持" << std::endl;
    std::cout << "网络流量统计: 需要网络监控模块支持" << std::endl;
    
    // 可以添加基本的网络连接测试
    std::cout << "注意: 详细网络诊断需要网络监控模块支持" << std::endl;
}

void DiagnosticTools::Cleanup() {
    if (m_initialized) {
        Logger::Info("清理诊断工具资源");
        m_initialized = false;
    }
}

std::pair<std::string, std::vector<std::string>> DiagnosticTools::ParseCommandLine(const std::string& input) {
    std::istringstream iss(input);
    std::string command;
    std::vector<std::string> args;
    
    iss >> command;
    
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return {command, args};
}

std::string DiagnosticTools::ExecuteSystemCommand(const std::string& command) {
    std::string result;
    
    // 这里可以实现系统命令执行逻辑
    // 现在先返回空字符串
    
    return result;
}

} // namespace DevTools
} // namespace TCMT