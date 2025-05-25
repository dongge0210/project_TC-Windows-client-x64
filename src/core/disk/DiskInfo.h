#pragma once
#include <vector>
#include <string>
#include <map>
#include "../DataStruct/DataStruct.h" // 只在这里包含DiskInfoData和DriveInfo的定义

// 注意：如果DataStruct.h中已定义DriveInfo和SmartAttribute，则不要在此重复定义。
// 若DataStruct.h未定义DriveInfo，则在此补充定义如下：
// struct DriveInfo {
//     char letter;
//     std::string label;
//     std::string fileSystem;
//     uint64_t totalSize;
//     uint64_t freeSpace;
//     uint64_t usedSpace;
// }

// 避免SmartAttribute重定义：只在此声明，不定义。
// struct SmartAttribute; // 前向声明（如有需要）

// 结构体：分区信息
struct PartitionInfo {
    std::string letter;         // 盘符
    std::string label;          // 卷标
    std::string fileSystem;     // 文件系统 (NTFS, FAT32, etc.)
    std::string partitionTable; // 分区表类型 (GPT/MBR)
    uint64_t totalSize;         // 总容量（字节）
    uint64_t usedSpace;         // 已用空间（字节）
};

class DiskInfo {
public:
    DiskInfo(); // Default constructor
    ~DiskInfo();

    // These methods will now read from Shared Memory
    static std::vector<PhysicalDiskInfo> GetAllPhysicalDisks();
    static std::vector<SmartAttribute> GetSmartAttributes(const std::string& physicalDiskName); // diskName can be the \\.\PHYSICALDRIVEx form

    // Keep existing methods for logical drives if they are still used and populate from shared memory
    static std::vector<DriveInfo> GetLogicalDrives();
    static std::string FormatSize(uint64_t bytes); // Helper, can be kept or moved

private:
    // Helper to convert shared memory string to std::string
    static std::string WcharArrayToString(const wchar_t* wstr, size_t maxLen);
};