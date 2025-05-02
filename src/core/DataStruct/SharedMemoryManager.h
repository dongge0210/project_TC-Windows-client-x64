#pragma once
#include <string>
#include <vector>
#include <windows.h>

// Forward declaration
struct SystemInfo;
struct SharedMemoryBlock;

// 共享内存区域结构
struct SharedMemoryRegion {
    std::string name;     // 共享内存名称
    HANDLE hMapFile;      // 文件映射句柄
    LPBYTE pBuffer;       // 缓冲区指针
    size_t size;          // 内存区域大小
};

// 共享内存管理类
class SharedMemoryManager {
private:
    // 跟踪所有创建的共享内存区域
    static std::vector<SharedMemoryRegion*> memoryRegions;
    
    // 全局共享内存实现
    static HANDLE hMapFile;
    static SharedMemoryBlock* pBuffer;
    static std::string lastError;

public:
    // 构造函数和析构函数
    SharedMemoryManager() = default;
    ~SharedMemoryManager();
    
    // 共享内存区域管理 - 对象接口
    SharedMemoryRegion* CreateSharedMemory(const std::string& name, size_t size);
    SharedMemoryRegion* ConnectToSharedMemory(const std::string& name, size_t size);
    bool WriteToSharedMemory(SharedMemoryRegion* region, const void* data, size_t size, size_t offset = 0);
    bool ReadFromSharedMemory(SharedMemoryRegion* region, void* buffer, size_t size, size_t offset = 0);
    void CloseSharedMemory(SharedMemoryRegion* region);
    
    // 系统级共享内存 - 静态接口
    static bool InitSharedMemory();
    static void CleanupSharedMemory();
    static bool WriteToSharedMemory(const SystemInfo& sysInfo);
    static SharedMemoryBlock* GetBuffer();
    static std::string GetSharedMemoryError();

    // Add a method to check if shared memory is initialized
    static bool IsSharedMemoryInitialized() {
        return pBuffer != nullptr;
    }

    // Add declaration for SetGlobalPrivilegeEnabled
    static void SetGlobalPrivilegeEnabled(bool enabled);
};
