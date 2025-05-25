#pragma once
#include <vector>
#include <string>
#include "../DataStruct/DataStruct.h" // 只在这里包含DiskInfoData的定义

// 结构体：分区信息
struct PartitionInfo {
    std::string letter;         // 盘符
    std::string label;          // 卷标
    std::string fileSystem;     // 文件系统 (NTFS, FAT32, etc.)
    std::string partitionTable; // 分区表类型 (GPT/MBR)
    uint64_t totalSize;         // 总容量（字节）
    uint64_t usedSpace;         // 已用空间（字节）
};

// 结构体：物理磁盘信息
struct PhysicalDiskInfo {
    std::string name;           // 物理磁盘名称
    std::string type;           // 磁盘类型 (SSD/HDD)
    std::string protocol;       // 连接协议 (SATA/NVMe/USB)
    uint64_t totalSize;         // 总容量（字节）
    std::string smartStatus;    // SMART状态
    std::vector<PartitionInfo> partitions; // 该物理磁盘下的分区
};

// DiskInfoData结构体定义已移除，统一在DataStruct.h中定义

class DiskInfo {
public:
    DiskInfo();
    void Refresh();
    const std::vector<DriveInfo>& GetDrives() const;

    // 静态方法：获取所有磁盘信息（对外统一接口）
    static std::vector<DiskInfoData> GetAllDisks();
    static std::vector<PhysicalDiskInfo> GetAllPhysicalDisks(); // 新增
    static std::string FormatSize(uint64_t size); // 格式化容量显示

private:
    void QueryDrives();
    std::vector<DriveInfo> drives;
};