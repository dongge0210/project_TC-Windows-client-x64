// DataStruct.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

#pragma pack(push, 1) // 确保内存对齐

// POD types for shared memory (no std::string, std::vector, etc.)

// GPU information structure for shared memory
struct GPUDataSM {
    wchar_t name[128];
    wchar_t brand[64];
    uint64_t vram;           // 专用显存 (VRAM)
    uint64_t sharedMemory;   // 可用共享内存
    uint32_t coreClock;
    float power;
};

// Network adapter information structure for shared memory
struct NetworkAdapterDataSM {
    wchar_t name[128];
    wchar_t mac[32];
    uint64_t speed;
};

// Disk information structure for shared memory
struct SharedDiskData {
    wchar_t letter;
    wchar_t label[128];
    wchar_t fileSystem[32];
    uint64_t totalSize;
    uint64_t usedSpace;
    uint64_t freeSpace;
};

// Temperature sensor data structure for shared memory
struct TemperatureData {
    wchar_t sensorName[128];
    float temperature;
};

// Shared memory block structure (POD only)
struct SharedMemoryBlock {
    CRITICAL_SECTION lock;

    // CPU information
    wchar_t cpuName[128];
    wchar_t cpuArch[64];
    int physicalCores;
    int logicalCores;
    float cpuUsage;
    int performanceCores;
    int efficiencyCores;
    double pCoreFreq;
    double eCoreFreq;
    bool hyperThreading;
    bool virtualization;
    float cpuPower;

    // Memory information
    uint64_t totalMemory;
    uint64_t usedMemory;
    uint64_t availableMemory;
    uint32_t memoryFrequency;

    // GPU information
    GPUDataSM gpus[2];
    int gpuCount;
    float gpuPower;
    float totalPower;

    // Network adapters
    NetworkAdapterDataSM adapters[4];
    int adapterCount;

    // Disk information
    SharedDiskData disks[8];
    int diskCount;

    // Temperature sensors
    TemperatureData temperatures[10];
    int tempCount;

    // System information
    wchar_t osDetailedVersion[256];
    SYSTEMTIME lastUpdate; 
    wchar_t motherboardName[128];
    wchar_t deviceName[128];
};

struct GPUData {
    std::string name;
    std::string brand;
    uint64_t vram;          // 专用显存
    uint64_t sharedMemory;  // 共享内存
    uint32_t coreClock;
};

struct NetworkAdapterData {
    std::string name;
    std::string mac;
    uint64_t speed;
};

struct DiskInfoData {
    char letter;
    std::string label;
    std::string fileSystem;
    uint64_t totalSize;
    uint64_t usedSpace;
    uint64_t freeSpace;
    bool isPhysical = false;
};

struct SystemInfo {
    std::string cpuName;
    int physicalCores;
    int logicalCores;
    double cpuUsage;
    int performanceCores;
    int efficiencyCores;
    double performanceCoreFreq;
    double efficiencyCoreFreq;
    bool hyperThreading;
    bool virtualization;
    uint64_t totalMemory;
    uint64_t usedMemory;
    uint64_t availableMemory;

    std::vector<GPUData> gpus;
    std::vector<NetworkAdapterData> adapters;
    std::vector<DiskInfoData> disks;
    std::vector<std::pair<std::string, double>> temperatures;
    std::string osVersion;
    std::string osDetailedVersion;
    std::string cpuArch;
    std::string motherboardName;
    std::string deviceName;
    double cpuPower;
    double gpuPower;
    double totalPower;
    uint32_t memoryFrequency;
    SYSTEMTIME lastUpdate;
};

struct SystemData {
    std::string cpuName;
    int physicalCores;
    int logicalCores;
    double cpuUsage;
    int performanceCores;
    int efficiencyCores;
    double performanceCoreFreq;
    double efficiencyCoreFreq;
    bool hyperThreading;
    bool virtualization;
    uint64_t totalMemory;
    uint64_t usedMemory;
    uint64_t availableMemory;
    std::vector<GPUData> gpus;
    std::vector<NetworkAdapterData> adapters;
    std::vector<DiskInfoData> disks;
    std::vector<std::pair<std::string, double>> temperatures;
    std::string osVersion;
    SYSTEMTIME lastUpdate;
};

#pragma pack(pop)