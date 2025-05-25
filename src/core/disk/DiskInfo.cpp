// DiskInfo.cpp
#include "DiskInfo.h"
#include "../DataStruct/DataStruct.h"
#include <windows.h>
#include <algorithm>
#include "../Utils/Logger.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

// 新增：单位换算辅助函数
static std::string FormatSize(uint64_t bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;
    char buf[64];
    if (bytes >= TB)
        snprintf(buf, sizeof(buf), "%.2f TB", bytes / TB);
    else if (bytes >= GB)
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / GB);
    else if (bytes >= MB)
        snprintf(buf, sizeof(buf), "%.2f MB", bytes / MB);
    else if (bytes >= KB)
        snprintf(buf, sizeof(buf), "%.2f KB", bytes / KB);
    else
        snprintf(buf, sizeof(buf), "%llu B", bytes);
    return buf;
}

// 简单格式化容量
std::string DiskInfo::FormatSize(uint64_t size) {
    double dsize = static_cast<double>(size);
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unit = 0;
    while (dsize >= 1024 && unit < 4) {
        dsize /= 1024;
        ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << dsize << units[unit];
    return oss.str();
}

// 构造函数实现
DiskInfo::DiskInfo() {
    QueryDrives();
}

// 查询所有本地磁盘
void DiskInfo::QueryDrives() {
    drives.clear();
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        std::wstring rootPath = std::wstring(1, drive) + L":\\";
        UINT driveType = GetDriveTypeW(rootPath.c_str());

        if (driveType == DRIVE_FIXED) {
            DriveInfo info;
            info.letter = drive;

            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExW(rootPath.c_str(),
                &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {

                info.totalSize = totalBytes.QuadPart;
                info.freeSpace = totalFreeBytes.QuadPart;
                info.usedSpace = totalBytes.QuadPart - totalFreeBytes.QuadPart;

                // 获取卷标
                wchar_t volumeName[MAX_PATH + 1] = { 0 };
                wchar_t fileSystemName[MAX_PATH + 1] = { 0 };
                GetVolumeInformationW(rootPath.c_str(),
                    volumeName, MAX_PATH,
                    NULL, NULL, NULL,
                    fileSystemName, MAX_PATH);

                info.label = volumeName;
                info.fileSystem = fileSystemName;

                drives.push_back(info);
            }
        }
    }
}

// 刷新磁盘信息
void DiskInfo::Refresh() {
    QueryDrives();
}

// 获取当前磁盘信息
const std::vector<DriveInfo>& DiskInfo::GetDrives() const {
    return drives;
}

// 静态方法：获取所有磁盘信息（名称、型号、总容量、可用空间）
std::vector<DiskInfoData> DiskInfo::GetAllDisks() {
    std::vector<DiskInfoData> disks;
    char driveStrings[256] = {0};
    DWORD len = GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);
    if (len == 0) {
        Logger::Error("无法获取逻辑磁盘列表");
        return disks;
    }

    for (char* drive = driveStrings; *drive; drive += strlen(drive) + 1) {
        UINT type = GetDriveTypeA(drive);
        if (type == DRIVE_FIXED) { // 只标记物理磁盘
            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExA(drive, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                DiskInfoData info;
                info.letter = drive[0];
                char volumeName[MAX_PATH] = {0};
                char fileSystemName[MAX_PATH] = {0};
                DWORD serialNumber = 0, maxComponentLen = 0, fileSystemFlags = 0;
                if (GetVolumeInformationA(
                        drive, volumeName, MAX_PATH, &serialNumber, &maxComponentLen,
                        &fileSystemFlags, fileSystemName, MAX_PATH)) {
                    info.label = volumeName;
                    info.fileSystem = fileSystemName;
                } else {
                    info.label = "";
                    info.fileSystem = "";
                }
                info.totalSize = totalBytes.QuadPart;
                info.freeSpace = freeBytesAvailable.QuadPart;
                info.usedSpace = info.totalSize > info.freeSpace ? (info.totalSize - info.freeSpace) : 0;
                info.isPhysical = true; // 只对物理磁盘设为true
                disks.push_back(info);
            }
        }
        // 其它类型磁盘不加入或isPhysical=false
    }
    if (disks.empty()) {
        Logger::Warning("未检测到任何磁盘");
    }
    return disks;
}

// 新增：获取所有物理磁盘及其分区信息
std::vector<PhysicalDiskInfo> DiskInfo::GetAllPhysicalDisks() {
    std::vector<PhysicalDiskInfo> result;
    // 这里仅为示例，实际应调用Windows API获取物理磁盘、分区、SMART等信息
    // 你可用DeviceIoControl、WMI等方式完善

    // 示例：模拟两个物理磁盘
    PhysicalDiskInfo disk1;
    disk1.name = "WDC WD10EZEX-08WN4A0";
    disk1.type = "HDD";
    disk1.protocol = "SATA";
    disk1.totalSize = 1000ull * 1024 * 1024 * 1024;
    disk1.smartStatus = "良好";

    PartitionInfo part1;
    part1.letter = "C:";
    part1.label = "系统";
    part1.fileSystem = "NTFS";
    part1.partitionTable = "GPT";
    part1.totalSize = 500ull * 1024 * 1024 * 1024;
    part1.usedSpace = 300ull * 1024 * 1024 * 1024;

    PartitionInfo part2;
    part2.letter = "D:";
    part2.label = "数据";
    part2.fileSystem = "NTFS";
    part2.partitionTable = "GPT";
    part2.totalSize = 500ull * 1024 * 1024 * 1024;
    part2.usedSpace = 100ull * 1024 * 1024 * 1024;

    disk1.partitions.push_back(part1);
    disk1.partitions.push_back(part2);

    result.push_back(disk1);

    // 你可以继续添加更多物理磁盘的真实采集逻辑
    return result;
}