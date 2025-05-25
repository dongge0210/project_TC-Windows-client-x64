#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <map>

#include "../DataStruct/DataStruct.h"

struct PartitionInfoBridge {
    std::string letter;
    std::string label;
    std::string fileSystem;
    std::string partitionTable;
    uint64_t totalSize;
    uint64_t usedSpace;
};

struct PhysicalDiskInfoBridge {
    std::string name;
    std::string model;
    std::string serialNumber;
    std::string firmwareRevision;
    uint64_t totalSize;
    std::string smartStatus;
    std::string protocol;
    std::string type;
    std::vector<PartitionInfoBridge> partitions;
    std::vector<SmartAttribute> smartAttributes;
};

/**
 * @brief LibreHardwareMonitor桥接类
 *
 * 只暴露纯C++接口，不出现任何CLR相关类型
 */
class LibreHardwareMonitorBridge {
public:
    static void Initialize();
    static void Cleanup();
    static std::vector<std::pair<std::string, double>> GetTemperatures();
    static double GetCpuPower();
    static double GetGpuPower();
    static double GetTotalPower();
    static std::vector<PhysicalDiskInfoBridge> GetPhysicalDisksWithSmart();
    static std::vector<SmartAttribute> GetSmartAttributes(const std::string& physicalDiskName);
private:
    static bool initialized;
    // 不在头文件暴露任何CLR类型和gcroot成员
};
