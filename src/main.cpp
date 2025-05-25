#include "core/cpu/CpuInfo.h"
#include "core/gpu/GpuInfo.h"
#include "core/memory/MemoryInfo.h"
#include "core/network/NetworkAdapter.h"
#include "core/os/OSInfo.h"
#include "core/utils/Logger.h"
#include "core/utils/TimeUtils.h"
#include "core/utils/WinUtils.h"
#include "core/temperature/LibreHardwareMonitorBridge.h"
#include "core/utils/WmiManager.h"
#include "core/disk/DiskInfo.h"
#include "core/DataStruct/DataStruct.h"
#include "core/DataStruct/SharedMemoryManager.h"  // Include the new shared memory manager
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <utility>
#include <thread>
#include <QMessageBox> 
#include <io.h>
#include <fcntl.h>
#include <msclr/marshal_cppstd.h>
#include <shellapi.h> // 新增：用于ShellExecute

// 保证没有与comutil.h冲突的全局符号、宏、using等
// 不要定义Data_t、operator=、operator+等与comutil.h同名的内容
// 不要定义GpuInfo(const char*)等与comutil.h构造函数冲突的内容

//QT已在本体软件因为COM证实为无效，QTUI将单独UI通过内存共享显示

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

//辅助函数
// 硬件名称翻译
std::string TranslateHardwareName(const std::string& name) {
    if (name.find("CPU Package") != std::string::npos) return "CPU温度";
    if (name.find("GPU Core") != std::string::npos) return "GPU温度";
    return name;
}

// 品牌判断
std::string GetGpuBrand(const std::wstring& name) {
    if (name.find(L"NVIDIA") != std::wstring::npos) return "NVIDIA";
    if (name.find(L"AMD") != std::wstring::npos) return "AMD";
    if (name.find(L"Intel") != std::wstring::npos) return "Intel";
    return "未知";
}

// 网络速度单位
std::string FormatNetworkSpeed(double speedBps) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    if (speedBps >= 1000000000) {
        ss << (speedBps / 1000000000) << " Gbps";
    }
    else if (speedBps >= 1000000) {
        ss << (speedBps / 1000000) << " Mbps";
    }
    else if (speedBps >= 1000) {
        ss << (speedBps / 1000) << " Kbps";
    }
    else {
        ss << speedBps << " bps";
    }
    return ss.str();
}

// 时间格式化
std::string FormatDateTime(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);  // 使用安全版本

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string FormatFrequency(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    if (value >= 1000) {
        ss << (value / 1000.0) << " GHz";
    }
    else {
        ss << value << " MHz";
    }
    return ss.str();
}

std::string FormatPercentage(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value << "%"; // 保留两位小数
    return ss.str();
}

std::string FormatTemperature(double value) {
    std::stringstream ss;
    ss << static_cast<int>(value) << "°C";  // 显示整数温度
    return ss.str();
}

std::string FormatSize(uint64_t bytes, bool useBinary = true) {
    const double kb = useBinary ? 1024.0 : 1000.0;
    const double mb = kb * kb;
    const double gb = mb * kb;
    const double tb = gb * kb;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    if (bytes >= tb) ss << (bytes / tb) << " TB";
    else if (bytes >= gb) ss << (bytes / gb) << " GB";
    else if (bytes >= mb) ss << (bytes / mb) << " MB";
    else if (bytes >= kb) ss << (bytes / kb) << " KB";
    else ss << bytes << " B";

    return ss.str();
}

std::string FormatDiskUsage(uint64_t used, uint64_t total) {
    if (total == 0) return "0%";
    double percentage = (static_cast<double>(used) / total) * 100.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << percentage << "%";
    return ss.str();
}

// 修正功率格式化：不要用百分号
std::string FormatPower(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value << " W";
    return ss.str();
}

static void PrintSectionHeader(const std::string& title) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 14); // 黄色
    std::cout << "\n=== " << title << " ===" << std::endl;
    SetConsoleTextAttribute(hConsole, 7); // 恢复默认颜色
}

static void PrintInfoItem(const std::string& label, const std::string& value, int indent = 2) {
    std::cout << std::string(indent, ' ')
        << std::left << std::setw(25) << label
        << ": " << value << std::endl;
}

// Remove InitSharedMemory and WriteToSharedMemory functions - now in SharedMemoryManager
// 将SystemInfo转换为共享内存格式
void ConvertToSharedData(const SystemInfo& sysInfo, SharedMemoryBlock& sharedData) {
    wcscpy_s(sharedData.cpuName, sizeof(sharedData.cpuName) / sizeof(wchar_t), WinUtils::StringToWstring(sysInfo.cpuName).c_str());
    wcscpy_s(sharedData.motherboardName, sizeof(sharedData.motherboardName) / sizeof(wchar_t),
        WinUtils::StringToWstring(sysInfo.motherboardName).c_str());
    wcscpy_s(sharedData.deviceName, sizeof(sharedData.deviceName) / sizeof(wchar_t),
        WinUtils::StringToWstring(sysInfo.deviceName).c_str());

    if (!sysInfo.gpus.empty()) {
        // Use the correct types and properly calculate the size for wcscpy_s
        wcscpy_s(sharedData.gpus[0].name, sizeof(sharedData.gpus[0].name) / sizeof(wchar_t),
                 WinUtils::StringToWstring(sysInfo.gpus[0].name).c_str());
        wcscpy_s(sharedData.gpus[0].brand, sizeof(sharedData.gpus[0].brand) / sizeof(wchar_t),
                 WinUtils::StringToWstring(sysInfo.gpus[0].brand).c_str());
    }
}

// 检查是否为管理员
bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// 请求以管理员权限重新启动自身
void RelaunchAsAdmin(int argc, char* argv[]) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // 构造命令行参数
    std::wstring cmdLine;
    for (int i = 1; i < argc; ++i) {
        cmdLine += L" \"";
        int len = MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, NULL, 0);
        std::wstring warg(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, &warg[0], len);
        cmdLine += warg.c_str();
        cmdLine += L"\"";
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = cmdLine.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        MessageBoxW(NULL, L"需要管理员权限，请右键以管理员身份运行。", L"权限不足", MB_ICONERROR);
    }
}

//主要函数
int main(int argc, char* argv[]) {
    // ====== 管理员权限检测与自提权 ======
    if (!IsRunAsAdmin()) {
        RelaunchAsAdmin(argc, argv);
        return 0;
    }

    // 在 main 函数开始处添加
    wchar_t runtimePath[MAX_PATH] = L"";
    GetEnvironmentVariableW(L"ProgramFiles", runtimePath, MAX_PATH);

    // 添加到DLL搜索路径
    AddDllDirectory(runtimePath);

    // 然后设置LibreHardwareMonitor DLL路径
    SetDllDirectory(L"F:\\Win_x64-10.lastest-sysMonitor\\src\\third_party\\LibreHardwareMonitor-0.9.4\\bin\\Debug\\net472");
    try {
        // Set console output to UTF-8
        _setmode(_fileno(stdout), _O_U8TEXT);

        Logger::EnableConsoleOutput(true); // Enable console output for Logger
        Logger::Initialize("system_monitor.log");
        Logger::Info("程序启动");

        // 初始化COM为多线程模式
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            if (hr == RPC_E_CHANGED_MODE) {
                Logger::Error("COM初始化模式冲突: 线程已初始化为不同的模式");
            }
            else {
                Logger::Error("COM初始化失败: 0x" + std::to_string(hr));
            }
            return -1;
        }

        struct ComCleanup {
            ~ComCleanup() { CoUninitialize(); }
        } comCleanup;

        // 检测管理员权限
        bool isAdmin = WinUtils::IsUserAdmin();
        Logger::Info(std::string("当前用户是否管理员: ") + (isAdmin ? "是" : "否"));

        // Set global privilege based on admin status
        SharedMemoryManager::SetGlobalPrivilegeEnabled(isAdmin);

        // Use SharedMemoryManager instead of local functions
        if (!SharedMemoryManager::InitSharedMemory()) {
            Logger::Error("共享内存初始化失败: " + SharedMemoryManager::GetSharedMemoryError());
            QMessageBox::critical(nullptr, "错误", "共享内存初始化失败，程序将退出。");
            return 1;
        }

        // 创建WMI管理器并初始化
        WmiManager wmiManager;
        if (!wmiManager.IsInitialized()) {
            Logger::Error("WMI初始化失败");
            QMessageBox::critical(nullptr, "错误", "WMI初始化失败，无法获取系统信息。");
            return 1;
        }

        try {
            LibreHardwareMonitorBridge::Initialize();
        }
        catch (System::IO::FileNotFoundException^ ex) {
            std::wstring wstr = msclr::interop::marshal_as<std::wstring>(ex->Message);
            Logger::Warning(wstr);
        }
        catch (System::Exception^ ex) {
            std::wstring wstr = msclr::interop::marshal_as<std::wstring>(ex->Message);
            Logger::Warning(wstr);
        }
        catch (const std::exception& e) {
            Logger::Warning("LibreHardwareMonitor 初始化失败 (C++ 异常): " + std::string(e.what()));
        }

        while (true) {

            // 获取系统信息
            SystemInfo sysInfo;

            // 操作系统信息
            OSInfo os;
            sysInfo.osVersion = os.GetVersion();
            sysInfo.osDetailedVersion = os.GetDetailedVersion(); // 新增

            // CPU信息
            CpuInfo cpu;
            sysInfo.cpuName = cpu.GetName();
            sysInfo.cpuArch = cpu.GetArchitecture(); // 新增
            sysInfo.physicalCores = cpu.GetLargeCores() + cpu.GetSmallCores();
            sysInfo.logicalCores = cpu.GetTotalCores();
            sysInfo.performanceCores = cpu.GetLargeCores();
            sysInfo.efficiencyCores = cpu.GetSmallCores();
            double cpuUsage = cpu.GetUsage();
            if (std::isnan(cpuUsage) || std::isinf(cpuUsage) || cpuUsage < 0) {
                cpuUsage = 0.0; // 使用默认值替代无效值
                Logger::Warning("获取到无效的 CPU 使用率数据，CPU使用率无效");
            }
            sysInfo.cpuUsage = cpuUsage;
            sysInfo.hyperThreading = cpu.IsHyperThreadingEnabled();
            sysInfo.virtualization = cpu.IsVirtualizationEnabled();
            sysInfo.performanceCoreFreq = cpu.GetLargeCoreSpeed();
            sysInfo.efficiencyCoreFreq = cpu.GetSmallCoreSpeed() * 0.8;

            sysInfo.cpuPower = LibreHardwareMonitorBridge::GetCpuPower();
            sysInfo.gpuPower = LibreHardwareMonitorBridge::GetGpuPower();
            sysInfo.totalPower = LibreHardwareMonitorBridge::GetTotalPower();

            // 内存信息
            MemoryInfo mem;
            sysInfo.totalMemory = mem.GetTotalPhysical();
            sysInfo.usedMemory = mem.GetTotalPhysical() - mem.GetAvailablePhysical();
            sysInfo.availableMemory = mem.GetAvailablePhysical();
            sysInfo.memoryFrequency = mem.GetMemoryFrequency(); // 新增：内存频率

            // GPU信息
            GpuInfo gpuInfo(wmiManager);
            sysInfo.gpus.clear();
            for (const auto& g : gpuInfo.GetGpuData()) {
                GPUData gd;
                gd.name = WinUtils::WstringToString(g.name);
                gd.brand = WinUtils::WstringToString(g.brand);
                gd.vram = g.vram;               // 正确传递专用显存
                gd.sharedMemory = g.sharedMemory; // 传递共享内存
                gd.coreClock = g.coreClock;
                // 新增：驱动版本
                gd.driverVersion = g.driverVersion;
                sysInfo.gpus.push_back(gd);
            }

            // 网络适配器信息
            NetworkAdapter network(wmiManager);
            sysInfo.adapters.clear();
            for (const auto& a : network.GetAdapters()) {
                NetworkAdapterData nd;
                nd.name = WinUtils::WstringToString(a.name);
                nd.mac = WinUtils::WstringToString(a.mac);
                nd.speed = a.speed;
                nd.ip = WinUtils::WstringToString(a.ip);       
                nd.connected = a.isConnected;                    
                sysInfo.adapters.push_back(nd);
            }


            auto temps = LibreHardwareMonitorBridge::GetTemperatures();
            sysInfo.temperatures = temps;

            // 只显示物理磁盘
            sysInfo.disks.clear();
            for (const auto& disk : DiskInfo::GetAllPhysicalDisks()) {
                // Convert PhysicalDiskInfo to DiskInfoData for sysInfo.disks
                DiskInfoData diskData;
                diskData.letter = 0; // or use a helper if you can map name to letter
                diskData.label = disk.name; // or disk.model if you prefer
                diskData.fileSystem = ""; // Not available in PhysicalDiskInfo
                diskData.totalSize = disk.totalSize;
                diskData.usedSpace = 0; // Not available in PhysicalDiskInfo
                diskData.freeSpace = 0; // Not available in PhysicalDiskInfo
                diskData.isPhysical = true;
                sysInfo.disks.push_back(diskData);
            }

            sysInfo.motherboardName = os.GetMotherboardName();
            sysInfo.deviceName = os.GetDeviceName();

            // 写入共享内存，使用 SharedMemoryManager 代替
            SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer(); // Get buffer once
            if (pBuffer) { // Ensure buffer is valid before writing
                auto physicalDisksFromBridge = LibreHardwareMonitorBridge::GetPhysicalDisksWithSmart();
                pBuffer->physicalDiskCountSM = static_cast<int>(physicalDisksFromBridge.size());
                if (pBuffer->physicalDiskCountSM > MAX_PHYSICAL_DISKS_SM) {
                    pBuffer->physicalDiskCountSM = MAX_PHYSICAL_DISKS_SM; // Cap at max
                    Logger::Warning("Too many physical disks detected, capping at " + std::to_string(MAX_PHYSICAL_DISKS_SM));
                }

                for (int i = 0; i < pBuffer->physicalDiskCountSM; ++i) {
                    const auto& bridgeDisk = physicalDisksFromBridge[i];
                    PhysicalDiskDataSM& smDisk = pBuffer->physicalDisksSM[i];

                    // Convert and copy PhysicalDiskInfoBridge to PhysicalDiskDataSM
                    wcsncpy_s(smDisk.name, sizeof(smDisk.name) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.name).c_str(), _TRUNCATE);
                    wcsncpy_s(smDisk.model, sizeof(smDisk.model) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.model).c_str(), _TRUNCATE);
                    wcsncpy_s(smDisk.serialNumber, sizeof(smDisk.serialNumber) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.serialNumber).c_str(), _TRUNCATE);
                    wcsncpy_s(smDisk.firmwareRevision, sizeof(smDisk.firmwareRevision) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.firmwareRevision).c_str(), _TRUNCATE);
                    smDisk.totalSize = bridgeDisk.totalSize;
                    wcsncpy_s(smDisk.smartStatus, sizeof(smDisk.smartStatus) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.smartStatus).c_str(), _TRUNCATE);
                    wcsncpy_s(smDisk.protocol, sizeof(smDisk.protocol) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.protocol).c_str(), _TRUNCATE);
                    wcsncpy_s(smDisk.type, sizeof(smDisk.type) / sizeof(wchar_t), WinUtils::StringToWstring(bridgeDisk.type).c_str(), _TRUNCATE);
                    
                    smDisk.smartAttributeCount = static_cast<int>(bridgeDisk.smartAttributes.size());
                    if (smDisk.smartAttributeCount > MAX_SMART_ATTRIBUTES_PER_DISK) {
                        smDisk.smartAttributeCount = MAX_SMART_ATTRIBUTES_PER_DISK; // Cap
                    }

                    for (int j = 0; j < smDisk.smartAttributeCount; ++j) {
                        const auto& bridgeAttr = bridgeDisk.smartAttributes[j];
                        // Additional processing for SMART attributes can be added here
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        Logger::Error("程序运行时发生异常: " + std::string(e.what()));
        QMessageBox::critical(nullptr, "错误", "程序运行时发生异常，程序将退出。");
        return 1;
    }

    return 0;
}
