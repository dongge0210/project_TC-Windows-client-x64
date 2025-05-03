// DiskInfo.h
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "../DataStruct/DataStruct.h"

// 新增：DriveInfo结构体定义
struct DriveInfo {
    char letter;                // 盘符
    std::wstring label;         // 卷标
    std::wstring fileSystem;    // 文件系统
    uint64_t totalSize;         // 总容量（字节）
    uint64_t usedSpace;         // 已用空间（字节）
    uint64_t freeSpace;         // 可用空间（字节）
};

class DiskInfo {
public:
    DiskInfo();

    // 成员方法
    void QueryDrives();
    void Refresh();
    const std::vector<DriveInfo>& GetDrives() const;

    // 静态方法
    static std::vector<DiskInfoData> GetAllDisks();

private:
    std::vector<DriveInfo> drives;
};

