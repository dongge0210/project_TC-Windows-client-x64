// DiskInfo.cpp
#include "DiskInfo.h"
#include "../DataStruct/SharedMemoryManager.h" // For accessing shared memory
#include "../Utils/Logger.h"
#include "../Utils/WinUtils.h" // For WstringToStdString and vice-versa if needed
#include "../DataStruct/DataStruct.h" // Ensure this is included for PhysicalDiskInfo

#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <comdef.h> // For _bstr_t (if still used, but likely not for shared memory parsing)

std::string DiskInfo::WcharArrayToString(const wchar_t* wstr, size_t maxLen) {
    if (!wstr) return "";
    size_t len = 0;
    while (len < maxLen && wstr[len] != L'\0') {
        len++;
    }
    return WinUtils::WstringToStdString(std::wstring(wstr, len));
}

DiskInfo::DiskInfo() {
    // Constructor might be empty if all data comes from shared memory via static methods
}

DiskInfo::~DiskInfo() {
    // Destructor
}

std::vector<PhysicalDiskInfo> DiskInfo::GetAllPhysicalDisks() {
    std::vector<PhysicalDiskInfo> disks;
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();

    if (!pBuffer) {
        Logger::Error("DiskInfo::GetAllPhysicalDisks - Shared memory buffer is null.");
        return disks;
    }

    for (int i = 0; i < pBuffer->physicalDiskCountSM; ++i) {
        const PhysicalDiskDataSM& smDisk = pBuffer->physicalDisksSM[i];
        PhysicalDiskInfo uiDisk;

        uiDisk.name = WcharArrayToString(smDisk.name, sizeof(smDisk.name) / sizeof(wchar_t));
        uiDisk.model = WcharArrayToString(smDisk.model, sizeof(smDisk.model) / sizeof(wchar_t));
        uiDisk.serialNumber = WcharArrayToString(smDisk.serialNumber, sizeof(smDisk.serialNumber) / sizeof(wchar_t));
        uiDisk.firmwareRevision = WcharArrayToString(smDisk.firmwareRevision, sizeof(smDisk.firmwareRevision) / sizeof(wchar_t));
        uiDisk.totalSize = smDisk.totalSize;
        uiDisk.smartStatus = WcharArrayToString(smDisk.smartStatus, sizeof(smDisk.smartStatus) / sizeof(wchar_t));
        uiDisk.protocol = WcharArrayToString(smDisk.protocol, sizeof(smDisk.protocol) / sizeof(wchar_t));
        uiDisk.type = WcharArrayToString(smDisk.type, sizeof(smDisk.type) / sizeof(wchar_t));

        for (int j = 0; j < smDisk.smartAttributeCount; ++j) {
            const SmartAttributeSM& smAttr = smDisk.smartAttributes[j];
            SmartAttribute uiAttr;
            uiAttr.id = smAttr.id;
            uiAttr.name = WcharArrayToString(smAttr.name, sizeof(smAttr.name) / sizeof(wchar_t));
            uiAttr.value = smAttr.value;
            uiAttr.worst = smAttr.worst;
            uiAttr.threshold = smAttr.threshold;
            uiAttr.raw = smAttr.rawValue;
            uiDisk.smartAttributes.push_back(uiAttr);
        }
        disks.push_back(uiDisk);
    }

    return disks;
}

std::vector<SmartAttribute> DiskInfo::GetSmartAttributes(const std::string& physicalDiskName) {
    std::vector<SmartAttribute> attributes;
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();

    if (!pBuffer) {
        Logger::Error("DiskInfo::GetSmartAttributes - Shared memory buffer is null.");
        return attributes;
    }

    for (int i = 0; i < pBuffer->physicalDiskCountSM; ++i) {
        const PhysicalDiskDataSM& smDisk = pBuffer->physicalDisksSM[i];
        std::string currentDiskName = WcharArrayToString(smDisk.name, sizeof(smDisk.name) / sizeof(wchar_t));

        if (currentDiskName == physicalDiskName) {
            for (int j = 0; j < smDisk.smartAttributeCount; ++j) {
                const SmartAttributeSM& smAttr = smDisk.smartAttributes[j];
                SmartAttribute uiAttr;
                uiAttr.id = smAttr.id;
                uiAttr.name = WcharArrayToString(smAttr.name, sizeof(smAttr.name) / sizeof(wchar_t));
                uiAttr.value = smAttr.value;
                uiAttr.worst = smAttr.worst;
                uiAttr.threshold = smAttr.threshold;
                uiAttr.raw = smAttr.rawValue;
                attributes.push_back(uiAttr);
            }
            break;
        }
    }

    return attributes;
}

std::vector<DriveInfo> DiskInfo::GetLogicalDrives() {
    std::vector<DriveInfo> drives;
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) {
        Logger::Error("DiskInfo::GetLogicalDrives - Shared memory buffer is null.");
        return drives;
    }

    for (int i = 0; i < pBuffer->diskCount; ++i) {
        const SharedDiskData& smDrive = pBuffer->disks[i];
        DriveInfo drive;
        drive.letter = static_cast<char>(smDrive.letter);
        drive.label = WcharArrayToString(smDrive.label, sizeof(smDrive.label) / sizeof(wchar_t));
        drive.fileSystem = WcharArrayToString(smDrive.fileSystem, sizeof(smDrive.fileSystem) / sizeof(wchar_t));
        drive.totalSize = smDrive.totalSize;
        drive.usedSpace = smDrive.usedSpace;
        drive.freeSpace = smDrive.freeSpace;
        drive.isPhysical = false;
        drives.push_back(drive);
    }

    return drives;
}

std::string DiskInfo::FormatSize(uint64_t bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (bytes >= TB) {
        oss << (static_cast<double>(bytes) / TB) << " TB";
    } else if (bytes >= GB) {
        oss << (static_cast<double>(bytes) / GB) << " GB";
    } else if (bytes >= MB) {
        oss << (static_cast<double>(bytes) / MB) << " MB";
    } else if (bytes >= KB) {
        oss << (static_cast<double>(bytes) / KB) << " KB";
    } else {
        oss << bytes << " B";
    }
    return oss.str();
}