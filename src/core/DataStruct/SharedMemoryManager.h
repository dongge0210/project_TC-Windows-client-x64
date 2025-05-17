#pragma once
#include <windows.h> // 保证头文件包含顺序正确，windows.h、Ole2.h等在comutil.h之前
#include <string>
#include <vector>
#include <Windows.h>
#include "DataStruct.h"

// Forward declaration
struct SystemInfo;
struct SharedMemoryBlock;
struct SystemData;
struct SharedMemoryRegion;

// SharedMemoryRegion structure definition
struct SharedMemoryRegion {
    std::string name;     // 共享内存名称
    size_t size;          // 内存区域大小
    HANDLE hMapFile;      // 文件映射句柄
    LPBYTE pBuffer;       // 缓冲区指针
};

// 共享内存管理类
class SharedMemoryManager {
public:
    // 系统级共享内存 - 静态接口
    static bool InitSharedMemory();
    static void CleanupSharedMemory();
    static bool WriteToSharedMemory(SystemInfo& sysInfo); // Remove const
    static void ReadSystemInfoFromSharedMemory(SystemInfo& sysInfo);
    static SharedMemoryBlock* GetBuffer();
    static std::string GetSharedMemoryError();

    // 新增：判断共享内存是否已初始化
    static bool IsSharedMemoryInitialized();

    // 内存区域操作
    static SharedMemoryRegion* CreateSharedMemory(const std::string& name, size_t size);
    static SharedMemoryRegion* ConnectToSharedMemory(const std::string& name, size_t size);
    static bool WriteToSharedMemory(SharedMemoryRegion* region, const void* data, size_t size, size_t offset = 0);
    static bool ReadFromSharedMemory(SharedMemoryRegion* region, void* buffer, size_t size, size_t offset = 0);
    static void CloseSharedMemory(SharedMemoryRegion* region);

    // 工具函数
    static void SetGlobalPrivilegeEnabled(bool enabled);
    static void UpdateDiskInfo();
    static SystemData& GetSystemData();

    ~SharedMemoryManager();

private:
    static HANDLE hMapFile;
    static SharedMemoryBlock* pBuffer;
    static std::string lastError;
    static std::vector<SharedMemoryRegion*> memoryRegions;
};