﻿// 首先，取消可能导致冲突的 Qt 宏定义
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// 取消可能导致冲突的 Qt 宏定义 (放在 Windows.h 之后)
#ifdef QT_VERSION
    // 暂时保存 Qt 宏定义
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

// 保证头文件包含顺序正确，windows.h、Ole2.h等在comutil.h之前
#include <windows.h>

// Make sure Windows.h is included before any other headers that might redefine GetLastError

#include "SharedMemoryManager.h"
#include "../DataStruct/DataStruct.h" // Include DataStruct.h for SharedMemoryBlock definition
#include "../Utils/WinUtils.h"
#include "../Utils/Logger.h"
#include "../disk/DiskInfo.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

static void ToGPUDataSM(const GPUInfo & src, GPUDataSM & dst) {
    wcsncpy_s(dst.name, 128, WinUtils::StringToWstring(src.name).c_str(), _TRUNCATE);
    wcsncpy_s(dst.brand, 64, WinUtils::StringToWstring(src.brand).c_str(), _TRUNCATE);
    dst.vram = src.vram;           // 专用显存
    dst.sharedMemory = src.sharedMemory; // 共享内存
    dst.coreClock = src.coreClock;
    wcsncpy_s(dst.driverVersion, 128, src.driverVersion.c_str(), _TRUNCATE);
    // 新增
    dst.available = src.available ? 1 : 0;
    wcsncpy_s(dst.status, 64, WinUtils::StringToWstring(src.status).c_str(), _TRUNCATE);
    dst.temperature = src.temperature;
}

static void ToNetworkAdapterDataSM(const NetworkAdapterData& src, NetworkAdapterDataSM& dst) {
    wcsncpy_s(dst.name, 128, WinUtils::StringToWstring(src.name).c_str(), _TRUNCATE);
    wcsncpy_s(dst.mac, 32, WinUtils::StringToWstring(src.mac).c_str(), _TRUNCATE);
    dst.speed = src.speed;
    wcsncpy_s(dst.ip, 64, WinUtils::StringToWstring(src.ip).c_str(), _TRUNCATE); // 新增
    dst.connected = src.connected; // 新增
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

// Helper function to convert SharedDiskData to DiskInfoData
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

// Helper function to convert GPUDataSM to GPUInfo
static GPUInfo SharedGPUToGPUData(const GPUDataSM& gpu) {
    GPUInfo gd;
    gd.name = WinUtils::WstringToString(gpu.name);
    gd.brand = WinUtils::WstringToString(gpu.brand);
    gd.vram = gpu.vram;           // 专用显存
    gd.sharedMemory = gpu.sharedMemory; // 共享内存
    gd.coreClock = gpu.coreClock;
    gd.driverVersion = gpu.driverVersion;
    // 新增
    gd.available = (gpu.available != 0);
    gd.status = WinUtils::WstringToString(gpu.status);
    gd.temperature = gpu.temperature;
    return gd;
}

// Helper function to convert NetworkAdapterDataSM to NetworkAdapterData
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
// Fallback implementation for FormatWindowsErrorMessage
inline std::string FallbackFormatWindowsErrorMessage(DWORD errorCode) {
    std::stringstream ss;
    ss << "错误代码: " << errorCode;
    return ss.str();
}
#endif

bool SharedMemoryManager::InitSharedMemory() {
    // 现在 IsProcessElevated() 已可用
    if (!WinUtils::IsProcessElevated()) {
        Logger::Warning("未检测到管理员权限，尝试使用全局共享内存。");
    } else {
        Logger::Info("检测到管理员权限，尝试使用 SeCreateGlobalPrivilege权限。");
        // Clear any previous error
        lastError.clear();
        
        try {
            // Try to enable privileges needed for creating global objects
            // 检查当前权限状态
            bool hasPrivilege = WinUtils::CheckPrivilege(L"SeCreateGlobalPrivilege");
            if (hasPrivilege) {
                Logger::Info("已具有 SeCreateGlobalPrivilege 权限");
            } else {
                // 尝试提升权限
                bool enableResult = WinUtils::EnablePrivilege(L"SeCreateGlobalPrivilege");
                if (!enableResult) {
                    DWORD errorCode = GetLastError();
                    std::stringstream ss;
                    ss << "未能启用 SeCreateGlobalPrivilege 权限，错误代码: " << errorCode
                       << " (" << FallbackFormatWindowsErrorMessage(errorCode) << ")";
                    Logger::Warning(ss.str());
                    Logger::Warning("尝试继续执行，但可能无法创建全局共享内存");
                } else {
                    Logger::Info("成功启用 SeCreateGlobalPrivilege 权限");
                }
            }
        } catch(const std::exception& e) {
            Logger::Warning(std::string("启用 SeCreateGlobalPrivilege 时发生异常: ") + e.what());
        }
    }

    // Attempt to create global shared memory
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

            // Fallback to local shared memory
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
                return false;
            }
        } else {
            std::stringstream ss;
            ss << "共享内存创建失败，错误代码: " << errorCode
               << " (" << FallbackFormatWindowsErrorMessage(errorCode) << ")";
            lastError = ss.str();
            Logger::Error(lastError);
            qDebug() << "[调试] 共享内存创建失败";
            return false;
        }
    }

    // Store GetLastError result in a DWORD variable before comparing
    DWORD errorCode = GetLastError(); // Direct call
    bool alreadyExists = (errorCode == ERROR_ALREADY_EXISTS);
    if (alreadyExists) {
        Logger::Info("已连接到现有共享内存");
    } else {
        Logger::Info("已创建新的共享内存");
    }

    // Map view of the file
    pBuffer = (SharedMemoryBlock*)MapViewOfFile(
        hMapFile,    // Handle to mapping object
        FILE_MAP_ALL_ACCESS,  // Read/write permission
        0,
        0,
        sizeof(SharedMemoryBlock));

    if (pBuffer == nullptr) {
        DWORD errorCode = GetLastError(); // Direct call
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
        return false;
    }

    // Initialize mutex if new shared memory was created
    if (!alreadyExists) {
        // Initialize critical section in the shared memory
        InitializeCriticalSection(&(pBuffer->lock));
        Logger::Info("共享内存互斥锁初始化完成");

        // Clear the memory
        ZeroMemory(pBuffer, sizeof(SharedMemoryBlock));

        // Re-initialize critical section (it was cleared by ZeroMemory)
        InitializeCriticalSection(&(pBuffer->lock));
    }

    Logger::Info("共享内存初始化完成");
    qDebug() << "[调试] 共享内存初始化完成";
    return true;
}

void SharedMemoryManager::CleanupSharedMemory() {
    if (pBuffer != nullptr) {
        // Delete the critical section before unmapping
        DeleteCriticalSection(&(pBuffer->lock));
        
        UnmapViewOfFile(pBuffer);
        pBuffer = nullptr;
    }

    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }
    
    Logger::Info("共享内存已清理");
}

bool SharedMemoryManager::WriteToSharedMemory(SystemInfo& sysInfo) {
    if (pBuffer == nullptr) {
        lastError = "共享内存未初始化";
        Logger::Error(lastError);
        return false;
    }

    try {
        EnterCriticalSection(&(pBuffer->lock));

        // Core CPU info
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

        // Memory info
        pBuffer->totalMemory = sysInfo.totalMemory;
        pBuffer->usedMemory = sysInfo.usedMemory;
        pBuffer->availableMemory = sysInfo.availableMemory;
        pBuffer->memoryFrequency = sysInfo.memoryFrequency;

        // GPU info
        int gpuCount = (std::min)(static_cast<int>(sysInfo.gpus.size()), 2);
        pBuffer->gpuCount = gpuCount;
        for (int i = 0; i < gpuCount; i++) {
            ToGPUDataSM(sysInfo.gpus[i], pBuffer->gpus[i]);
        }

        // OS info
        wcsncpy_s(pBuffer->osDetailedVersion, _countof(pBuffer->osDetailedVersion), WinUtils::StringToWstring(sysInfo.osDetailedVersion).c_str(), _TRUNCATE);
        wcsncpy_s(pBuffer->motherboardName, _countof(pBuffer->motherboardName), WinUtils::StringToWstring(sysInfo.motherboardName).c_str(), _TRUNCATE);
        wcsncpy_s(pBuffer->deviceName, _countof(pBuffer->deviceName), WinUtils::StringToWstring(sysInfo.deviceName).c_str(), _TRUNCATE);

        // GPU power
        pBuffer->gpuPower = static_cast<float>(sysInfo.gpuPower);
        pBuffer->totalPower = static_cast<float>(sysInfo.totalPower);

        // Network adapters info
        int adapterCount = (std::min)(static_cast<int>(sysInfo.adapters.size()), 4);
        pBuffer->adapterCount = adapterCount;
        for (int i = 0; i < adapterCount; i++) {
            ToNetworkAdapterDataSM(sysInfo.adapters[i], pBuffer->adapters[i]);
        }

        // Disk info
        int diskCount = (std::min)(static_cast<int>(sysInfo.disks.size()), 8);
        pBuffer->diskCount = diskCount;
        for (int i = 0; i < diskCount; i++) {
            ToSharedDiskData(sysInfo.disks[i], pBuffer->disks[i]);
        }

        // Temperature info
        int tempCount = (std::min)(static_cast<int>(sysInfo.temperatures.size()), 10);
        pBuffer->tempCount = tempCount;
        for (int i = 0; i < tempCount; i++) {
            ToTemperatureData(sysInfo.temperatures[i], pBuffer->temperatures[i]);
        }

        // Update timestamp
        GetSystemTime(&(pBuffer->lastUpdate));

        LeaveCriticalSection(&(pBuffer->lock));
        return true;
    }
    catch (const std::exception& e) {
        if (pBuffer != nullptr) {
            LeaveCriticalSection(&(pBuffer->lock));
        }
        lastError = std::string("写入共享内存时异常: ") + e.what();
        Logger::Error(lastError);
        return false;
    }
    catch (...) {
        if (pBuffer != nullptr) {
            LeaveCriticalSection(&(pBuffer->lock));
        }
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

    try {
        EnterCriticalSection(&(pBuffer->lock));

        // Core CPU info
        sysInfo.cpuName = WinUtils::WstringToString(pBuffer->cpuName);
        sysInfo.cpuArch = WinUtils::WstringToString(pBuffer->cpuArch);
        sysInfo.physicalCores = pBuffer->physicalCores;
        sysInfo.logicalCores = pBuffer->logicalCores;
        sysInfo.cpuUsage = static_cast<double>(pBuffer->cpuUsage);
        sysInfo.performanceCores = pBuffer->performanceCores;
        sysInfo.efficiencyCores = pBuffer->efficiencyCores;
        sysInfo.performanceCoreFreq = pBuffer->pCoreFreq * 1000; // GHz to MHz
        sysInfo.efficiencyCoreFreq = pBuffer->eCoreFreq * 1000; // GHz to MHz
        sysInfo.hyperThreading = pBuffer->hyperThreading;
        sysInfo.virtualization = pBuffer->virtualization;
        sysInfo.cpuPower = static_cast<double>(pBuffer->cpuPower);

        // Memory info
        sysInfo.totalMemory = pBuffer->totalMemory;
        sysInfo.usedMemory = pBuffer->usedMemory;
        sysInfo.availableMemory = pBuffer->availableMemory;
        sysInfo.memoryFrequency = pBuffer->memoryFrequency;

        // GPU info
        sysInfo.gpus.clear();
        for (int i = 0; i < pBuffer->gpuCount; i++) {
            sysInfo.gpus.push_back(SharedGPUToGPUData(pBuffer->gpus[i]));
        }

        // OS info
        sysInfo.osDetailedVersion = WinUtils::WstringToString(pBuffer->osDetailedVersion);
        sysInfo.motherboardName = WinUtils::WstringToString(pBuffer->motherboardName);
        sysInfo.deviceName = WinUtils::WstringToString(pBuffer->deviceName);

        // GPU power
        sysInfo.gpuPower = static_cast<double>(pBuffer->gpuPower);
        sysInfo.totalPower = static_cast<double>(pBuffer->totalPower);

        // Network adapters info
        sysInfo.adapters.clear();
        for (int i = 0; i < pBuffer->adapterCount; i++) {
            sysInfo.adapters.push_back(SharedAdapterToAdapterData(pBuffer->adapters[i]));
        }

        // Disk info
        sysInfo.disks.clear();
        for (int i = 0; i < pBuffer->diskCount; i++) {
            sysInfo.disks.push_back(SharedDiskToDiskInfoData(pBuffer->disks[i]));
        }

        // Temperature info
        sysInfo.temperatures.clear();
        for (int i = 0; i < pBuffer->tempCount; i++) {
            std::string sensorName = WinUtils::WstringToString(pBuffer->temperatures[i].sensorName);
            double temperature = static_cast<double>(pBuffer->temperatures[i].temperature);
            sysInfo.temperatures.emplace_back(sensorName, temperature);
        }

        LeaveCriticalSection(&(pBuffer->lock));
    }
    catch (const std::exception& e) {
        if (pBuffer != nullptr) {
            LeaveCriticalSection(&(pBuffer->lock));
        }
        lastError = std::string("读取共享内存时异常: ") + e.what();
        Logger::Error(lastError);
    }
    catch (...) {
        if (pBuffer != nullptr) {
            LeaveCriticalSection(&(pBuffer->lock));
        }
        lastError = "读取共享内存时发生未知异常";
        Logger::Error(lastError);
    }
}

SharedMemoryBlock* SharedMemoryManager::GetBuffer() {
    if (pBuffer != nullptr) {
        qDebug() << "[调试] 成功获取共享内存Buffer";
        return pBuffer;
    } else {
        qDebug() << "[调试] 获取共享内存Buffer失败";
        return nullptr;
    }
}

std::string SharedMemoryManager::GetSharedMemoryError() {
    return lastError;
}

// CreateSharedMemory implementation for memory regions
SharedMemoryRegion* SharedMemoryManager::CreateSharedMemory(const std::string& name, size_t size) {
    // 创建一个新的共享内存区域
    SharedMemoryRegion* region = new SharedMemoryRegion();
    region->name = name;
    region->size = size;
    
    std::string fullName = "Global\\" + name;  // 尝试创建全局内存
    bool isGlobal = true;
    
    // 尝试创建全局共享内存
    region->hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,              // 使用页面文件
        NULL,                              // 默认安全属性
        PAGE_READWRITE,                    // 读写访问
        0,                                 // 高位大小
        (DWORD)size,                       // 低位大小
        fullName.c_str()                   // 内存名称
    );
    
    // 检查是否创建失败
    DWORD error = GetLastError(); // Direct call
    if (region->hMapFile == NULL) {
        // 如果是权限问题（错误码5），尝试创建局部共享内存
        if (error == ERROR_ACCESS_DENIED) {
            Logger::Warning("创建全局共享内存失败，尝试创建局部共享内存");
            isGlobal = false;
            fullName = name;  // 不带Global前缀
            
            region->hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,      // 使用页面文件
                NULL,                      // 默认安全属性
                PAGE_READWRITE,            // 读写访问
                0,                         // 高位大小
                (DWORD)size,               // 低位大小
                fullName.c_str()           // 内存名称
            );
            
            error = GetLastError(); // Direct call
            if (region->hMapFile == NULL) {
                std::stringstream ss;
                ss << "局部共享内存创建失败，错误代码: " << error;
                Logger::Error(ss.str());
                delete region;
                return nullptr;
            }
        } else {
            std::stringstream ss;
            ss << "共享内存创建失败，错误代码: " << error;
            Logger::Error(ss.str() + " (错误代码: " + std::to_string(error) + ")");
            delete region;
            return nullptr;
        }
    } else if (error == ERROR_ALREADY_EXISTS) {
        Logger::Info(fullName + " 共享内存已存在，连接到现有内存");
    }
    
    // 映射共享内存视图
    region->pBuffer = (LPBYTE)MapViewOfFile(
        region->hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );
    
    if (region->pBuffer == NULL) {
        error = GetLastError(); // Direct call
        std::stringstream ss;
        ss << "无法映射共享内存视图，错误代码: " << error;
        Logger::Error(ss.str());
        CloseHandle(region->hMapFile);
        delete region;
        return nullptr;
    }
    
    // 将内存区域添加到列表中
    memoryRegions.push_back(region);
    
    if (isGlobal) {
        Logger::Info("成功创建全局共享内存: " + fullName);
    } else {
        Logger::Info("成功创建局部共享内存: " + fullName);
    }
    
    return region;
}

// ConnectToSharedMemory implementation for memory regions
SharedMemoryRegion* SharedMemoryManager::ConnectToSharedMemory(const std::string& name, size_t size) {
    // 创建一个新的共享内存区域
    SharedMemoryRegion* region = new SharedMemoryRegion();
    region->name = name;
    region->size = size;
    
    // 首先尝试全局命名空间
    std::string fullName = "Global\\" + name;
    region->hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        fullName.c_str()
    );
    
    // 如果全局命名空间失败，尝试局部命名空间
    if (region->hMapFile == NULL) {
        DWORD error = GetLastError(); // Direct call
        fullName = name;  // 不带Global前缀
        
        region->hMapFile = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            fullName.c_str()
        );
        
        if (region->hMapFile == NULL) {
            DWORD error2 = GetLastError(); // Direct call
            std::stringstream ss;
            ss << "无法连接到共享内存，全局错误: " << error << "，局部错误: " << error2;
            Logger::Error(ss.str());
            delete region;
            return nullptr;
        } else {
            Logger::Info("已连接到局部共享内存: " + fullName);
        }
    } else {
        Logger::Info("已连接到全局共享内存: " + fullName);
    }
    
    // 映射共享内存视图
    region->pBuffer = (LPBYTE)MapViewOfFile(
        region->hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );
    
    if (region->pBuffer == NULL) {
        DWORD error = GetLastError(); // Direct call
        std::stringstream ss;
        ss << "无法映射共享内存视图，错误代码: " << error;
        Logger::Error(ss.str());
        CloseHandle(region->hMapFile);
        delete region;
        return nullptr;
    }
    
    // 将内存区域添加到列表中
    memoryRegions.push_back(region);
    return region;
}

// WriteToSharedMemory implementation for memory regions
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
    
    // 复制数据到共享内存
    memcpy(region->pBuffer + offset, data, size);
    return true;
}

// ReadFromSharedMemory implementation for memory regions
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
    
    // 从共享内存复制数据
    memcpy(buffer, region->pBuffer + offset, size);
    return true;
}

// CloseSharedMemory implementation for memory regions
void SharedMemoryManager::CloseSharedMemory(SharedMemoryRegion* region) {
    if (region == nullptr) {
        return;
    }
    
    // 在内存区域列表中查找并移除
    auto it = std::find(memoryRegions.begin(), memoryRegions.end(), region);
    if (it != memoryRegions.end()) {
        memoryRegions.erase(it);
    }
    
    // 取消映射视图和关闭句柄
    if (region->pBuffer != nullptr) {
        UnmapViewOfFile(region->pBuffer);
    }
    
    if (region->hMapFile != NULL) {
        CloseHandle(region->hMapFile);
    }
    
    delete region;
}

// Destructor implementation
SharedMemoryManager::~SharedMemoryManager() {
    // 复制一份列表，因为CloseSharedMemory会修改原列表
    auto regionsCopy = memoryRegions;
    for (auto* region : regionsCopy) {
        CloseSharedMemory(region);
    }
    
    memoryRegions.clear();
}

void SharedMemoryManager::SetGlobalPrivilegeEnabled(bool enabled) {
    if (enabled) {
        Logger::Info("已为共享内存启用全局权限。");
    } else {
        Logger::Info("全局权限已禁用以用于共享内存。");
    }
}

void SharedMemoryManager::UpdateDiskInfo() {
    auto disks = DiskInfo::GetAllPhysicalDisks();
    std::vector<DiskInfoData> diskDataList;
    for (const auto& disk : disks) {
        DiskInfoData diskData;
        diskData.letter = 0; // or map if possible
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

// 新增：判断共享内存是否已初始化
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
