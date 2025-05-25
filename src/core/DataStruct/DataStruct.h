// DataStruct.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

#pragma pack(push, 1) // 确保内存对齐

// Define maximums for shared memory arrays
#define MAX_SMART_ATTRIBUTES_PER_DISK 30
#define MAX_PHYSICAL_DISKS_SM 4

// POD types for shared memory (no std::string, std::vector, etc.)

// GPU information structure for shared memory
struct GPUDataSM {
    wchar_t name[128];
    wchar_t brand[64];
    uint64_t vram;           // 专用显存 (VRAM)
    uint64_t sharedMemory;   // 可用共享内存
    double coreClock;        // 修改: 核心时钟类型为 double
    float power;
    wchar_t driverVersion[128];   // 新增: 驱动版本
    wchar_t driverDate[128];      // 新增: 驱动日期
    wchar_t driverProvider[128];  // 新增: 驱动提供商
};

// Network adapter information structure for shared memory
struct NetworkAdapterDataSM {
    wchar_t name[128];
    wchar_t mac[32];
    uint64_t speed;
    bool connected; // 新增
    wchar_t ip[64]; // 建议同步支持IP显示
};

// Disk information structure for shared memory (Logical Disks/Partitions)
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

// SMART Attribute structure for shared memory
struct SmartAttributeSM {
    int id;
    wchar_t name[64]; // Ensure sufficient size
    int value;
    int worst;
    int threshold;
    long long rawValue; // Changed from 'raw' to avoid conflict, ensure type matches
};

// Physical Disk information structure for shared memory, including SMART attributes
struct PhysicalDiskDataSM {
    wchar_t name[128];          // e.g., "\\\\.\\PHYSICALDRIVE0"
    wchar_t model[128];         // Disk model
    wchar_t serialNumber[128];  // Serial number
    wchar_t firmwareRevision[64]; // Firmware revision
    uint64_t totalSize;         // Total size in bytes
    wchar_t smartStatus[64];    // e.g., "OK", "Pred Fail"
    wchar_t protocol[32];       // e.g., "NVMe", "SATA"
    wchar_t type[32];           // e.g., "Fixed", "Removable"

    int smartAttributeCount;
    SmartAttributeSM smartAttributes[MAX_SMART_ATTRIBUTES_PER_DISK];
    // Note: Partitions are not included here for simplicity,
    // but could be added if needed in shared memory.
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
    GPUDataSM gpus[2]; // 保证driverVersion同步
    int gpuCount;
    float gpuPower;
    float totalPower;

    // Network adapters
    NetworkAdapterDataSM adapters[4];
    int adapterCount;

    // Disk information (Logical Disks/Partitions)
    SharedDiskData disks[8]; // This is for logical drives
    int diskCount;           // Count of logical drives

    // Temperature sensors
    TemperatureData temperatures[10];
    int tempCount;

    // System information
    wchar_t osDetailedVersion[256];
    SYSTEMTIME lastUpdate; 
    wchar_t motherboardName[128];
    wchar_t deviceName[128];

    // NEW: Physical Disk and SMART information
    int physicalDiskCountSM; // Renamed to avoid conflict if physicalDiskCount is used elsewhere
    PhysicalDiskDataSM physicalDisksSM[MAX_PHYSICAL_DISKS_SM];

    // Placeholder for future refactoring to include SMART data
    /*
    int physicalDiskCount;
    PhysicalDiskDataSm disksSm[MAX_PHYSICAL_DISKS_SM]; // Define MAX_PHYSICAL_DISKS_SM
    */
};

struct GPUData {
    std::string name;
    std::string brand;
    uint64_t vram = 0;          // 修改: 默认值为 0
    uint64_t sharedMemory = 0;  // 修改: 默认值为 0
    double coreClock = 0;       // 修改: 核心时钟类型为 double，默认值为 0
    std::wstring driverVersion;   // 修改: 驱动版本
    std::wstring driverDate;      // 修改: 驱动日期
    std::wstring driverProvider;  // 修改: 驱动提供商
};

struct NetworkAdapterData {
    std::string name;
    std::string mac;
    uint64_t speed;
    bool connected;
    std::string ip;
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

// 确保DriveInfo和SmartAttribute只在此定义一次，供DiskInfo.h等头文件引用
struct DriveInfo {
    char letter;
    std::string label;
    std::string fileSystem;
    uint64_t totalSize;
    uint64_t usedSpace;
    uint64_t freeSpace;
    bool isPhysical;
};

#ifndef SMART_ATTRIBUTE_STRUCT_DEFINED
#define SMART_ATTRIBUTE_STRUCT_DEFINED
struct SmartAttribute {
    int id;
    std::string name;
    int value;
    int worst;
    int threshold;
    long long raw; // Keep as 'raw' for std::vector<SmartAttribute>
};
#endif

// This is the structure DiskInfo.h (UI side) will work with.
// It should be similar to PhysicalDiskInfoBridge but use std::string.
// We'll ensure DiskInfo.cpp populates this from PhysicalDiskDataSM.
struct PhysicalDiskInfo {
    std::string name; // e.g., "\\\\.\\PHYSICALDRIVE0" or a friendlier name like "Samsung SSD 970 EVO"
    std::string model;
    std::string serialNumber;
    std::string firmwareRevision;
    std::string type;     // e.g., "Fixed", "Removable"
    std::string protocol; // e.g., "NVMe", "SATA"
    uint64_t totalSize;
    std::string smartStatus; // e.g., "OK", "Pred Fail"
    std::vector<SmartAttribute> smartAttributes;
    // Partitions could be a std::vector<PartitionInfo> if needed by UI directly on PhysicalDiskInfo
    // For now, assuming partitions are handled separately or via logical disks.
};

#pragma pack(pop)