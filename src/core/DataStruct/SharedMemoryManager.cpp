// 首先，取消可能导致冲突的 Qt 宏定义
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// 取消可能导致冲突的 Qt 宏定义 (放在 Windows.h 之后)
#ifdef QT_VERSION
#define SAVE_QT_KEYWORDS
#ifdef slots
#undef slots
#endif
#ifdef signals
#undef signals
#endif
#ifdef emit
#undef emit
#endif
#ifdef for each (object var in collection_to_loop)
{

}
#undef foreach
#endif
#ifdef name
#undef name
#endif
#ifdef first
#undef first
#endif
#ifdef c_str
#undef c_str
#endif
#endif

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#include <algorithm>
#include "SharedMemoryManager.h"
#include "../DataStruct/DataStruct.h"
#include "../Utils/WinUtils.h"
#include "../Utils/Logger.h"
#include "../disk/DiskInfo.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

// -------------------- 命名互斥体相关 --------------------
static HANDLE g_hMutex = NULL;
constexpr const wchar_t* kSharedMemoryMutexName = L"Global\\SystemMonitorSharedMemoryMutex";

static bool InitMutex() {
    g_hMutex = CreateMutexW(NULL, FALSE, kSharedMemoryMutexName);
    return g_hMutex != NULL;
}

static void CleanupMutex() {
    if (g_hMutex) {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
}

static bool LockSharedMemory() {
    if (!g_hMutex) return false;
    DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);
    return waitResult == WAIT_OBJECT_0;
}

static void UnlockSharedMemory() {
    if (g_hMutex) ReleaseMutex(g_hMutex);
}

// -------------------------------------------------------

static void ToGPUDataSM(const GPUData& src, GPUDataSM& dst) {
    wcsncpy_s(dst.name, _countof(dst.name), WinUtils::StringToWstring(src.name).c_str(), _TRUNCATE);
    wcsncpy_s(dst.brand, _countof(dst.brand), WinUtils::StringToWstring(src.brand).c_str(), _TRUNCATE);
    dst.vram = src.vram;
    dst.sharedMemory = src.sharedMemory;
    dst.coreClock = src.coreClock;
    wcsncpy_s(dst.driverVersion, _countof(dst.driverVersion), src.driverVersion.c_str(), _TRUNCATE);
    dst.available = src.available ? 1 : 0;
    wcsncpy_s(dst.status, _countof(dst.status), WinUtils::StringToWstring(src.status).c_str(), _TRUNCATE);
    dst.temperature = src.temperature;
}

static void ToNetworkAdapterDataSM(const NetworkAdapterData& src, NetworkAdapterDataSM& dst) {
    wcsncpy_s(dst.name, 128, WinUtils::StringToWstring(src.name).c_str(), _TRUNCATE);
    wcsncpy_s(dst.mac, 32, WinUtils::StringToWstring(src.mac).c_str(), _TRUNCATE);
    dst.speed = src.speed;
    wcsncpy_s(dst.ip, 64, WinUtils::StringToWstring(src.ip).c_str(), _TRUNCATE);
    dst.connected = src.connected;
}

static void ToSharedDiskData(const DiskInfoData& src, SharedDiskData& dst) {
    dst.letter = src.letter;
    wcsncpy_s(dst.label, 128, WinUtils::StringToWstring(src.label).c_str(), _TRUNCATE);
    wcsncpy_s(dst.fileSystem, 32, WinUtils::StringToWstring(src.fileSystem).c_str(), _TRUNCATE);
    dst.totalSize = src.totalSize;
    dst.usedSpace = src.usedSpace;
    dst.freeSpace = src.freeSpace;
}

static void ToTemperatureData(const std::pair<std::string, double>& src, TemperatureData& dst) {
    wcsncpy_s(dst.sensorName, 128, WinUtils::StringToWstring(src.first).c_str(), _TRUNCATE);
    dst.temperature = static_cast<float>(src.second);
}

static DiskInfoData SharedDiskToDiskInfoData(const SharedDiskData& disk) {
    DiskInfoData info;
    info.letter = disk.letter;
    info.label = WinUtils::WstringToString(disk.label);
    info.fileSystem = WinUtils::WstringToString(disk.fileSystem);
    info.totalSize = disk.totalSize;
    info.usedSpace = disk.usedSpace;
    info.freeSpace = disk.freeSpace;
    return info;
}

static GPUData SharedGPUToGPUData(const GPUDataSM& gpu) {
    GPUData gd;
    gd.name = WinUtils::WstringToString(gpu.name);
    gd.brand = WinUtils::WstringToString(gpu.brand);
    gd.vram = gpu.vram;
    gd.sharedMemory = gpu.sharedMemory;
    gd.coreClock = gpu.coreClock;
    gd.driverVersion.assign(gpu.driverVersion);
    gd.available = (gpu.available != 0);
    gd.status = WinUtils::WstringToString(gpu.status);
    gd.temperature = gpu.temperature;
    return gd;
}

static NetworkAdapterData SharedAdapterToAdapterData(const NetworkAdapterDataSM& adapter) {
    NetworkAdapterData nd;
    nd.name = WinUtils::WstringToString(adapter.name);
    nd.mac = WinUtils::WstringToString(adapter.mac);
    nd.speed = adapter.speed;
    nd.ip = WinUtils::WstringToString(adapter.ip);
    nd.connected = adapter.connected;
    return nd;
}

// Initialize static members
HANDLE SharedMemoryManager::hMapFile = NULL;
SharedMemoryBlock* SharedMemoryManager::pBuffer = nullptr;
std::string SharedMemoryManager::lastError = "";
std::vector<SharedMemoryRegion*> SharedMemoryManager::memoryRegions;

// 全局SystemData对象只定义一次
static SystemData g_systemData;

#ifndef WINUTILS_IMPLEMENTED
inline std::string FallbackFormatWindowsErrorMessage(DWORD errorCode) {
    std::stringstream ss;
    ss << "错误代码: " << errorCode;
    return ss.str();
}
#endif

bool SharedMemoryManager::InitSharedMemory() {
    if (!InitMutex()) {
        lastError = "无法创建/打开命名互斥体";
        Logger::Error(lastError);
        return false;
    }

    if (!WinUtils::IsProcessElevated()) {
        Logger::Warning("未检测到管理员权限，尝试使用全局共享内存。");
    }
    else {
        Logger::Info("检测到管理员权限，尝试使用 SeCreateGlobalPrivilege权限。");
        lastError.clear();
        try {
            bool hasPrivilege = WinUtils::CheckPrivilege(L"SeCreateGlobalPrivilege");
            if (hasPrivilege) {
                Logger::Info("已具有 SeCreateGlobalPrivilege 权限");
            }
            else {
                bool enableResult = WinUtils::EnablePrivilege(L"SeCreateGlobalPrivilege");
                if (!enableResult) {
                    DWORD errorCode = GetLastError();
                    std::stringstream ss;
                    ss << "未能启用 SeCreateGlobalPrivilege 权限，错误代码: " << errorCode
                        << " (" << FallbackFormatWindowsErrorMessage(errorCode) << ")";
                    Logger::Warning(ss.str());
                    Logger::Warning("尝试继续执行，但可能无法创建全局共享内存");
                }
                else {
                    Logger::Info("成功启用 SeCreateGlobalPrivilege 权限");
                }
            }
        }
        catch (const std::exception& e) {
            Logger::Warning(std::string("启用 SeCreateGlobalPrivilege 时发生异常: ") + e.what());
        }
    }

    hMapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemoryBlock),
        L"Global\\SystemMonitorSharedMemory"
    );

    if (hMapFile == NULL) {
        DWORD errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED) {
            Logger::Warning("全局共享内存创建失败，尝试创建局部共享内存");
            hMapFile = CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                sizeof(SharedMemoryBlock),
                L"Local\\SystemMonitorSharedMemory"
            );
            if (hMapFile == NULL) {
                errorCode = GetLastError();
                std::stringstream ss;
                ss << "局部共享内存创建失败，错误代码: " << errorCode
                    << " (" << FallbackFormatWindowsErrorMessage(errorCode) << ")";
                lastError = ss.str();
                Logger::Error(lastError);
                qDebug() << "[调试] 局部共享内存创建失败";
                CleanupMutex();
                return false;
            }
        }
        else {
            std::stringstream ss;
            ss << "共享内存创建失败，错误代码: " << errorCode
                << " (" << FallbackFormatWindowsErrorMessage(errorCode) << ")";
            lastError = ss.str();
            Logger::Error(lastError);
            qDebug() << "[调试] 共享内存创建失败";
            CleanupMutex();
            return false;
        }
    }

    DWORD errorCode = GetLastError();
    bool alreadyExists = (errorCode == ERROR_ALREADY_EXISTS);
    if (alreadyExists) {
        Logger::Info("已连接到现有共享内存");
    }
    else {
        Logger::Info("已创建新的共享内存");
    }

    pBuffer = (SharedMemoryBlock*)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemoryBlock));

    if (pBuffer == nullptr) {
        DWORD errorCode = GetLastError();
        std::stringstream ss;
        ss << "无法访问共享内存，错误代码: " << errorCode
            << " ("
#ifdef WINUTILS_IMPLEMENTED
            << WinUtils::FormatWindowsErrorMessage(errorCode)
#else
            << FallbackFormatWindowsErrorMessage(errorCode)
#endif
            << ")";
        lastError = ss.str();
        Logger::Error(lastError);
        qDebug() << "[调试] 无法访问共享内存";
        CloseHandle(hMapFile);
        hMapFile = NULL;
        CleanupMutex();
        return false;
    }

    if (!alreadyExists) {
        ZeroMemory(pBuffer, sizeof(SharedMemoryBlock));
    }

    Logger::Info("共享内存初始化完成");
    qDebug() << "[调试] 共享内存初始化完成";
    return true;
}

void SharedMemoryManager::CleanupSharedMemory() {
    if (pBuffer != nullptr) {
        UnmapViewOfFile(pBuffer);
        pBuffer = nullptr;
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }
    CleanupMutex();
    Logger::Info("共享内存已清理");
}

bool SharedMemoryManager::WriteToSharedMemory(SystemInfo& sysInfo) {
    if (pBuffer == nullptr) {
        lastError = "共享内存未初始化";
        Logger::Error(lastError);
        return false;
    }

    if (!LockSharedMemory()) {
        lastError = "加锁共享内存失败";
        Logger::Error(lastError);
        return false;
    }

    try {
        wcsncpy_s(pBuffer->cpuName, _countof(pBuffer->cpuName), WinUtils::StringToWstring(sysInfo.cpuName).c_str(), _TRUNCATE);
        wcsncpy_s(pBuffer->cpuArch, _countof(pBuffer->cpuArch), WinUtils::StringToWstring(sysInfo.cpuArch).c_str(), _TRUNCATE);
        pBuffer->physicalCores = sysInfo.physicalCores;
        pBuffer->logicalCores = sysInfo.logicalCores;
        pBuffer->cpuUsage = static_cast<float>(sysInfo.cpuUsage);
        pBuffer->performanceCores = sysInfo.performanceCores;
        pBuffer->efficiencyCores = sysInfo.efficiencyCores;
        pBuffer->pCoreFreq = sysInfo.performanceCoreFreq / 1000.0;
        pBuffer->eCoreFreq = sysInfo.efficiencyCoreFreq / 1000.0;
        pBuffer->hyperThreading = sysInfo.hyperThreading;
        pBuffer->virtualization = sysInfo.virtualization;
        pBuffer->cpuPower = static_cast<float>(sysInfo.cpuPower);

        pBuffer->totalMemory = sysInfo.totalMemory;
        pBuffer->usedMemory = sysInfo.usedMemory;
        pBuffer->availableMemory = sysInfo.availableMemory;
        pBuffer->memoryFrequency = sysInfo.memoryFrequency;

        int gpuCount = (std::min)(static_cast<int>(sysInfo.gpus.size()), 2);
        pBuffer->gpuCount = gpuCount;
        for (int i = 0; i < gpuCount; i++) {
            ToGPUDataSM(sysInfo.gpus[i], pBuffer->gpus[i]);
        }

        wcsncpy_s(pBuffer->osDetailedVersion, _countof(pBuffer->osDetailedVersion), WinUtils::StringToWstring(sysInfo.osDetailedVersion).c_str(), _TRUNCATE);
        wcsncpy_s(pBuffer->motherboardName, _countof(pBuffer->motherboardName), WinUtils::StringToWstring(sysInfo.motherboardName).c_str(), _TRUNCATE);
        wcsncpy_s(pBuffer->deviceName, _countof(pBuffer->deviceName), WinUtils::StringToWstring(sysInfo.deviceName).c_str(), _TRUNCATE);

        pBuffer->gpuPower = static_cast<float>(sysInfo.gpuPower);
        pBuffer->totalPower = static_cast<float>(sysInfo.totalPower);

        int adapterCount = (std::min)(static_cast<int>(sysInfo.adapters.size()), 4);
        pBuffer->adapterCount = adapterCount;
        for (int i = 0; i < adapterCount; i++) {
            ToNetworkAdapterDataSM(sysInfo.adapters[i], pBuffer->adapters[i]);
        }

        int diskCount = (std::min)(static_cast<int>(sysInfo.disks.size()), 8);
        pBuffer->diskCount = diskCount;
        for (int i = 0; i < diskCount; i++) {
            ToSharedDiskData(sysInfo.disks[i], pBuffer->disks[i]);
        }

        int tempCount = (std::min)(static_cast<int>(sysInfo.temperatures.size()), 10);
        pBuffer->tempCount = tempCount;
        for (int i = 0; i < tempCount; i++) {
            ToTemperatureData(sysInfo.temperatures[i], pBuffer->temperatures[i]);
        }

        GetSystemTime(&(pBuffer->lastUpdate));
        wcsncpy_s(pBuffer->lastUpdateUtc,
            _countof(pBuffer->lastUpdateUtc),
            L"UTC not available",
            _TRUNCATE);

        UnlockSharedMemory();
        return true;
    }
    catch (const std::exception& e) {
        UnlockSharedMemory();
        lastError = std::string("写入共享内存时异常: ") + e.what();
        Logger::Error(lastError);
        return false;
    }
    catch (...) {
        UnlockSharedMemory();
        lastError = "写入共享内存时发生未知异常";
        Logger::Error(lastError);
        return false;
    }
}

void SharedMemoryManager::ReadSystemInfoFromSharedMemory(SystemInfo& sysInfo) {
    if (pBuffer == nullptr) {
        lastError = "共享内存未初始化";
        Logger::Error(lastError);
        return;
    }

    if (!LockSharedMemory()) {
        lastError = "加锁共享内存失败";
        Logger::Error(lastError);
        return;
    }

    try {
        sysInfo.cpuName = WinUtils::WstringToString(pBuffer->cpuName);
        sysInfo.cpuArch = WinUtils::WstringToString(pBuffer->cpuArch);
        sysInfo.physicalCores = pBuffer->physicalCores;
        sysInfo.logicalCores = pBuffer->logicalCores;
        sysInfo.cpuUsage = static_cast<double>(pBuffer->cpuUsage);
        sysInfo.performanceCores = pBuffer->performanceCores;
        sysInfo.efficiencyCores = pBuffer->efficiencyCores;
        sysInfo.performanceCoreFreq = pBuffer->pCoreFreq * 1000;
        sysInfo.efficiencyCoreFreq = pBuffer->eCoreFreq * 1000;
        sysInfo.hyperThreading = pBuffer->hyperThreading;
        sysInfo.virtualization = pBuffer->virtualization;
        sysInfo.cpuPower = static_cast<double>(pBuffer->cpuPower);

        sysInfo.totalMemory = pBuffer->totalMemory;
        sysInfo.usedMemory = pBuffer->usedMemory;
        sysInfo.availableMemory = pBuffer->availableMemory;
        sysInfo.memoryFrequency = pBuffer->memoryFrequency;

        sysInfo.gpus.clear();
        for (int i = 0; i < pBuffer->gpuCount; i++) {
            sysInfo.gpus.push_back(SharedGPUToGPUData(pBuffer->gpus[i]));
        }

        sysInfo.osDetailedVersion = WinUtils::WstringToString(pBuffer->osDetailedVersion);
        sysInfo.motherboardName = WinUtils::WstringToString(pBuffer->motherboardName);
        sysInfo.deviceName = WinUtils::WstringToString(pBuffer->deviceName);

        sysInfo.gpuPower = static_cast<double>(pBuffer->gpuPower);
        sysInfo.totalPower = static_cast<double>(pBuffer->totalPower);

        sysInfo.adapters.clear();
        for (int i = 0; i < pBuffer->adapterCount; i++) {
            sysInfo.adapters.push_back(SharedAdapterToAdapterData(pBuffer->adapters[i]));
        }

        sysInfo.disks.clear();
        for (int i = 0; i < pBuffer->diskCount; i++) {
            sysInfo.disks.push_back(SharedDiskToDiskInfoData(pBuffer->disks[i]));
        }

        sysInfo.temperatures.clear();
        for (int i = 0; i < pBuffer->tempCount; i++) {
            std::string sensorName = WinUtils::WstringToString(pBuffer->temperatures[i].sensorName);
            double temperature = static_cast<double>(pBuffer->temperatures[i].temperature);
            sysInfo.temperatures.emplace_back(sensorName, temperature);
        }

        UnlockSharedMemory();
    }
    catch (const std::exception& e) {
        UnlockSharedMemory();
        lastError = std::string("读取共享内存时异常: ") + e.what();
        Logger::Error(lastError);
    }
    catch (...) {
        UnlockSharedMemory();
        lastError = "读取共享内存时发生未知异常";
        Logger::Error(lastError);
    }
}

SharedMemoryBlock* SharedMemoryManager::GetBuffer() {
    if (pBuffer != nullptr) {
        qDebug() << "[调试] 成功获取共享内存Buffer";
        return pBuffer;
    }
    else {
        qDebug() << "[调试] 获取共享内存Buffer失败";
        return nullptr;
    }
}

std::string SharedMemoryManager::GetSharedMemoryError() {
    return lastError;
}

SharedMemoryRegion* SharedMemoryManager::CreateSharedMemory(const std::string& name, size_t size) {
    SharedMemoryRegion* region = new SharedMemoryRegion();
    region->name = name;
    region->size = size;

    std::string fullName = "Global\\" + name;
    bool isGlobal = true;

    region->hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        (DWORD)size,
        fullName.c_str()
    );

    DWORD error = GetLastError();
    if (region->hMapFile == NULL) {
        if (error == ERROR_ACCESS_DENIED) {
            Logger::Warning("创建全局共享内存失败，尝试创建局部共享内存");
            isGlobal = false;
            fullName = name;
            region->hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                (DWORD)size,
                fullName.c_str()
            );
            error = GetLastError();
            if (region->hMapFile == NULL) {
                std::stringstream ss;
                ss << "局部共享内存创建失败，错误代码: " << error;
                Logger::Error(ss.str());
                delete region;
                return nullptr;
            }
        }
        else {
            std::stringstream ss;
            ss << "共享内存创建失败，错误代码: " << error;
            Logger::Error(ss.str() + " (错误代码: " + std::to_string(error) + ")");
            delete region;
            return nullptr;
        }
    }
    else if (error == ERROR_ALREADY_EXISTS) {
        Logger::Info(fullName + " 共享内存已存在，连接到现有内存");
    }

    region->pBuffer = (LPBYTE)MapViewOfFile(
        region->hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );

    if (region->pBuffer == NULL) {
        error = GetLastError();
        std::stringstream ss;
        ss << "无法映射共享内存视图，错误代码: " << error;
        Logger::Error(ss.str());
        CloseHandle(region->hMapFile);
        delete region;
        return nullptr;
    }

    memoryRegions.push_back(region);

    if (isGlobal) {
        Logger::Info("成功创建全局共享内存: " + fullName);
    }
    else {
        Logger::Info("成功创建局部共享内存: " + fullName);
    }

    return region;
}

SharedMemoryRegion* SharedMemoryManager::ConnectToSharedMemory(const std::string& name, size_t size) {
    SharedMemoryRegion* region = new SharedMemoryRegion();
    region->name = name;
    region->size = size;

    std::string fullName = "Global\\" + name;
    region->hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        fullName.c_str()
    );

    if (region->hMapFile == NULL) {
        DWORD error = GetLastError();
        fullName = name;
        region->hMapFile = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            fullName.c_str()
        );
        if (region->hMapFile == NULL) {
            DWORD error2 = GetLastError();
            std::stringstream ss;
            ss << "无法连接到共享内存，全局错误: " << error << "，局部错误: " << error2;
            Logger::Error(ss.str());
            delete region;
            return nullptr;
        }
        else {
            Logger::Info("已连接到局部共享内存: " + fullName);
        }
    }
    else {
        Logger::Info("已连接到全局共享内存: " + fullName);
    }

    region->pBuffer = (LPBYTE)MapViewOfFile(
        region->hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );

    if (region->pBuffer == NULL) {
        DWORD error = GetLastError();
        std::stringstream ss;
        ss << "无法映射共享内存视图，错误代码: " << error;
        Logger::Error(ss.str());
        CloseHandle(region->hMapFile);
        delete region;
        return nullptr;
    }

    memoryRegions.push_back(region);
    return region;
}

bool SharedMemoryManager::WriteToSharedMemory(SharedMemoryRegion* region, const void* data, size_t size, size_t offset) {
    if (region == nullptr || region->pBuffer == nullptr) {
        Logger::Error("无效的共享内存区域");
        return false;
    }

    if (offset + size > region->size) {
        std::stringstream ss;
        ss << "写入超出范围: 偏移量=" << offset << ", 写入大小=" << size << ", 内存区域大小=" << region->size;
        Logger::Error(ss.str());
        return false;
    }

    memcpy(region->pBuffer + offset, data, size);
    return true;
}

bool SharedMemoryManager::ReadFromSharedMemory(SharedMemoryRegion* region, void* buffer, size_t size, size_t offset) {
    if (region == nullptr || region->pBuffer == nullptr) {
        Logger::Error("无效的共享内存区域");
        return false;
    }

    if (offset + size > region->size) {
        std::stringstream ss;
        ss << "读取超出范围: 偏移量=" << offset << ", 读取大小=" << size << ", 内存区域大小=" << region->size;
        Logger::Error(ss.str());
        return false;
    }

    memcpy(buffer, region->pBuffer + offset, size);
    return true;
}

void SharedMemoryManager::CloseSharedMemory(SharedMemoryRegion* region) {
    if (region == nullptr) {
        return;
    }

    auto it = std::find(memoryRegions.begin(), memoryRegions.end(), region);
    if (it != memoryRegions.end()) {
        memoryRegions.erase(it);
    }

    if (region->pBuffer != nullptr) {
        UnmapViewOfFile(region->pBuffer);
    }

    if (region->hMapFile != NULL) {
        CloseHandle(region->hMapFile);
    }

    delete region;
}

SharedMemoryManager::~SharedMemoryManager() {
    auto regionsCopy = memoryRegions;
    for (auto* region : regionsCopy) {
        CloseSharedMemory(region);
    }
    memoryRegions.clear();
}

void SharedMemoryManager::SetGlobalPrivilegeEnabled(bool enabled) {
    if (enabled) {
        Logger::Info("已为共享内存启用全局权限。");
    }
    else {
        Logger::Info("全局权限已禁用以用于共享内存。");
    }
}

void SharedMemoryManager::UpdateDiskInfo() {
    auto disks = DiskInfo::GetAllPhysicalDisks();
    std::vector<DiskInfoData> diskDataList;
    for (const auto& disk : disks) {
        DiskInfoData diskData;
        diskData.letter = 0;
        diskData.label = disk.name;
        diskData.fileSystem = "";
        diskData.totalSize = disk.totalSize;
        diskData.usedSpace = 0;
        diskData.freeSpace = 0;
        diskData.isPhysical = true;
        diskDataList.push_back(diskData);
    }
    g_systemData.disks = diskDataList;
}

SystemData& SharedMemoryManager::GetSystemData() {
    return g_systemData;
}

bool SharedMemoryManager::IsSharedMemoryInitialized() {
    return pBuffer != nullptr;
}

// 重新恢复 Qt 宏定义
#ifdef SAVE_QT_KEYWORDS
#ifdef signals
#define signals Q_SIGNALS
#endif
#ifdef slots
#define slots Q_SLOTS
#endif
#ifdef emit
#define emit Q_EMIT
#endif
#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif