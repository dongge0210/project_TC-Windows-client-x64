#pragma once
#include <vector>
#include <string>
#include "../DataStruct/DataStruct.h" // 只在这里包含DiskInfoData的定义

// 结构体：分区/卷信息
struct DriveInfo {
    char letter;                    // 盘符
    std::wstring label;             // 卷标
    std::wstring fileSystem;        // 文件系统
    uint64_t totalSize;             // 总容量（字节）
    uint64_t usedSpace;             // 已用空间（字节）
    uint64_t freeSpace;             // 可用空间（字节）
};

// DiskInfoData结构体定义已移除，统一在DataStruct.h中定义

class DiskInfo {
public:
    DiskInfo();
    void Refresh();
    const std::vector<DriveInfo>& GetDrives() const;

    // 静态方法：获取所有磁盘信息（对外统一接口）
    static std::vector<DiskInfoData> GetAllDisks();

private:
    void QueryDrives();
    std::vector<DriveInfo> drives;
};