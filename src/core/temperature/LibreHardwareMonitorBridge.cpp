#include "LibreHardwareMonitorBridge.h"
#include "Logger.h"
#include <msclr/marshal_cppstd.h>
#include <iostream>
#include <windows.h>
#include <limits> // Add for NaN
#include "../gpu/GpuInfo.h" // 新增：包含GpuInfo.h
#include <codecvt>
#include <locale>
#include <algorithm> // 新增：安全字符串转换和清理工具
#include <sstream> // 修复 stringstream 未定义
#include <string> // 确保string可见
#include "../DataStruct/DataStruct.h" // 包含DataStruct.h
#using "..\\src\\third_party\\LibreHardwareMonitor\\bin\\Debug\\net472\\LibreHardwareMonitorLib.dll"

using namespace LibreHardwareMonitor::Hardware;
using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;

// ===== NVML minimal declarations (no nvml.h) =====
typedef int nvmlReturn_t;
#define NVML_SUCCESS 0
extern "C" {
    nvmlReturn_t nvmlInit();
    const char* nvmlErrorString(nvmlReturn_t result);
}
// ================================================

// CLR相关静态成员和类型只在cpp文件内声明
static msclr::gcroot<Computer^> computer;
static msclr::gcroot<IVisitor^> visitor;

// Define the static member - ensure this is the only definition.
bool LibreHardwareMonitorBridge::initialized = false;
bool LibreHardwareMonitorBridge::nvmlEnabled = false;

public ref class UpdateVisitor : public IVisitor
{
public:
    // Ensure 'override' specifier is removed from all methods here
    virtual void VisitComputer(IComputer^ computer) {}
    virtual void VisitHardware(IHardware^ hardware)
    {
        hardware->Update();
        for each (IHardware ^ subHardware in hardware->SubHardware)
            subHardware->Accept(this);
    }
    virtual void VisitSensor(ISensor^ sensor) {}
    virtual void VisitParameter(IParameter^ parameter) {}
};

// 辅助函数：去除字符串前后空白和不可见字符
static std::string CleanString(const std::string& input) {
    std::string s = input;
    // 去除前后空白
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    // 替换不可见字符为普通空格
    std::replace_if(s.begin(), s.end(), [](unsigned char ch) { return (ch == '\t' || ch == '\r' || ch == '\n' || ch == '\xa0'); }, ' ');
    return s;
}

// 在初始化时输出所有硬件类型和名称，辅助调试
void LibreHardwareMonitorBridge::Initialize() {
    try {
        if (initialized) return;
        computer = gcnew Computer();
        computer->IsCpuEnabled = true;
        computer->IsGpuEnabled = true;
        computer->IsMotherboardEnabled = true;
        computer->IsControllerEnabled = true;
        computer->IsMemoryEnabled = true;
        computer->IsStorageEnabled = true;
        computer->IsNetworkEnabled = true;
        computer->IsBatteryEnabled = true;
        computer->Open();
        visitor = gcnew ::UpdateVisitor();
        initialized = true;
        Logger::Info(L"LibreHardwareMonitor 初始化完成");

        // 新增：输出所有硬件类型和名称（UTF-8安全转换并清理）
        for each (IHardware ^ hardware in computer->Hardware) {
            // 使用ToString()并marshal_as转换，避免Data()错误
            std::wstring hwTypeW = msclr::interop::marshal_as<std::wstring>(hardware->HardwareType.ToString());
            std::wstring hwNameW = msclr::interop::marshal_as<std::wstring>(hardware->Name);
            // 转为UTF-8
            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
            std::string hwType = CleanString(conv.to_bytes(hwTypeW));
            std::string hwName = CleanString(conv.to_bytes(hwNameW));
            Logger::Info("枚举到硬件: 类型=" + hwType + ", 名称=" + hwName); // 去掉[调试]前缀

            // 如果包含问号，输出原始宽字节内容用于调试
            if (hwName.find('?') != std::string::npos) {
                std::ostringstream ss;
                ss << "[调试] 原始硬件名称宽字节: ";
                for (wchar_t ch : hwNameW) {
                    if (ch == 0) break;
                    ss << std::hex << std::showbase << (int)ch << " ";
                }
                Logger::Info(ss.str());
            }
        }

        // 只保留调试，不再自动诊断
        // DiagnoseAllHardwareAndSensors(); // 注释掉未定义函数

        // 静态调用
        LibreHardwareMonitorBridge::InitNVML();
    }
    catch (System::IO::FileNotFoundException^ ex) {
        Logger::Error(msclr::interop::marshal_as<std::wstring>(ex->Message));
    }
    catch (System::Exception^ ex) {
        // 输出详细的.NET异常信息
        std::wstring msg = msclr::interop::marshal_as<std::wstring>(ex->ToString());
        Logger::Error(L"LibreHardwareMonitor 初始化异常: " + msg);
        std::wstring msg2 = msclr::interop::marshal_as<std::wstring>(ex->Message);
        Logger::Error(L"异常Message: " + msg2);
    }
}

void LibreHardwareMonitorBridge::Cleanup() {
    if (!initialized) return;
    computer->Close();
    computer = nullptr;
    visitor = nullptr;
    initialized = false;
    LibreHardwareMonitorBridge::nvmlEnabled = false;
}

// 静态实现
bool LibreHardwareMonitorBridge::InitNVML() {
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        Logger::Info("NVML 初始化失败: " + std::string(nvmlErrorString(result)));
        LibreHardwareMonitorBridge::nvmlEnabled = false;
        return false;
    }
    LibreHardwareMonitorBridge::nvmlEnabled = true;
    Logger::Info("NVML 初始化成功");
    return true;
}

void LibreHardwareMonitorBridge::EnsureNVML() {
    if (!LibreHardwareMonitorBridge::nvmlEnabled) {
        LibreHardwareMonitorBridge::InitNVML();
    }
}

std::vector<std::pair<std::string, double>> LibreHardwareMonitorBridge::GetTemperatures() {
    std::vector<std::pair<std::string, double>> temps;
    if (!initialized) return temps;

    computer->Accept(visitor);

    bool cpuTempAdded = false;
    bool gpuTempAdded = false;

    for each (IHardware ^ hardware in computer->Hardware) {
        hardware->Update();

        if (hardware->HardwareType == HardwareType::Cpu && !cpuTempAdded) {
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature) {
                    double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : std::numeric_limits<double>::quiet_NaN();
                    temps.push_back({ "CPU温度", value });
                    cpuTempAdded = true;
                    break; // 只取第一个有效CPU温度
                }
            }
        }
        else if ((hardware->HardwareType == HardwareType::GpuNvidia ||
                  hardware->HardwareType == HardwareType::GpuAmd ||
                  hardware->HardwareType == HardwareType::GpuIntel) && !gpuTempAdded) {
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature) {
                    std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                    double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : std::numeric_limits<double>::quiet_NaN();
                    // 只取"GPU Core"或"GPU温度"等主温度
                    if (name == "GPU Core" || name == "GPU温度") {
                        temps.push_back({ "GPU温度", value });
                        gpuTempAdded = true;
                        break; // 只取第一个有效GPU温度
                    }
                }
            }
        }
    }
    return temps;
}

// 移除LogAllPowerSensors的详细Logger::Info输出，仅保留maxValue逻辑
void LogAllPowerSensors(IHardware^ hardware, std::string prefix, double& maxValue, std::string& maxName) {
    hardware->Update();
    for each (ISensor ^ sensor in hardware->Sensors) {
        if (sensor->SensorType == SensorType::Power) {
            double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : -1.0;
            bool hasValue = sensor->Value.HasValue;
            std::string name = prefix + msclr::interop::marshal_as<std::string>(sensor->Name);
            if (hasValue && value > maxValue) {
                maxValue = value;
                maxName = name;
            }
        }
    }
    for each (IHardware ^ sub in hardware->SubHardware) {
        LogAllPowerSensors(sub, prefix + msclr::interop::marshal_as<std::string>(sub->Name) + "/", maxValue, maxName);
    }
}

// 移除LogAllSensors的所有Logger::Info输出
void LogAllSensors(IHardware^ hardware, std::string prefix) {
    hardware->Update();
    for each (IHardware ^ sub in hardware->SubHardware) {
        LogAllSensors(sub, prefix + msclr::interop::marshal_as<std::string>(sub->Name) + "/");
    }
}

double LibreHardwareMonitorBridge::GetCpuPower() {
    if (!initialized) return 0.0;
    computer->Accept(visitor);

    double maxCpuPower = 0.0;
    std::string maxName;
    bool anyPowerSensor = false;
    for each (IHardware ^ hardware in computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            LogAllPowerSensors(hardware, "", maxCpuPower, maxName);
            anyPowerSensor = true;
        }
    }
    return maxCpuPower;
}

double LibreHardwareMonitorBridge::GetGpuPower() {
    // 优先NVML
    LibreHardwareMonitorBridge::EnsureNVML();
    if (LibreHardwareMonitorBridge::nvmlEnabled) {
        double nvmlPower = GpuInfo::GetGpuPowerNVML();
        if (nvmlPower > 0 && !std::isnan(nvmlPower)) {
            Logger::Info("[采集][NVML] GPU Power: " + std::to_string(nvmlPower) + " W");
            return nvmlPower;
        }
    }

    // NVML无效，尝试LibreHardwareMonitor
    if (!initialized) return std::numeric_limits<double>::quiet_NaN();
    computer->Accept(visitor);

    double maxGpuPower = 0.0;
    std::string maxName;
    for each (IHardware ^ hardware in computer->Hardware) {
        if (hardware->HardwareType == HardwareType::GpuNvidia ||
            hardware->HardwareType == HardwareType::GpuAmd ||
            hardware->HardwareType == HardwareType::GpuIntel) {
            LogAllPowerSensors(hardware, "", maxGpuPower, maxName);
        }
    }
    if (maxGpuPower > 0) {
        Logger::Info("[采集][Libre] GPU Power: " + std::to_string(maxGpuPower) + " W");
        return maxGpuPower;
    }

    // 都没有
    return std::numeric_limits<double>::quiet_NaN();
}

double LibreHardwareMonitorBridge::GetTotalPower() {
    if (!initialized) return 0.0;
    computer->Accept(visitor);

    double maxTotalPower = 0.0;
    std::string maxName;
    bool anyPowerSensor = false;
    for each (IHardware ^ hardware in computer->Hardware) {
        LogAllPowerSensors(hardware, "", maxTotalPower, maxName);
        anyPowerSensor = true;
    }
    return maxTotalPower;
}

// 获取所有物理磁盘及其SMART信息
std::vector<PhysicalDiskInfoBridge> LibreHardwareMonitorBridge::GetPhysicalDisksWithSmart() {
    std::vector<PhysicalDiskInfoBridge> result;
    if (!initialized) return result;
    computer->Accept(visitor);

    for each (IHardware ^ hardware in computer->Hardware) {
        // 只处理物理磁盘
        if (hardware->HardwareType == HardwareType::Storage) {
            PhysicalDiskInfoBridge info;
            info.name = msclr::interop::marshal_as<std::string>(hardware->Name);
            // 类型、协议等信息无法直接获取，留空或用占位符
            info.type = "";      // 可通过WMI或其它方式补充
            info.protocol = "";  // 可通过WMI或其它方式补充
            // 容量通过Sensors查找
            info.totalSize = 0;
            info.smartStatus = ""; // SMART健康状态无法直接获取，后续可通过ISmart接口补充

            // 遍历Sensors，查找容量相关信息
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Data) {
                    std::string sname = msclr::interop::marshal_as<std::string>(sensor->Name);
                    if (sname.find("Capacity") != std::string::npos && sensor->Value.HasValue) {
                        info.totalSize = static_cast<uint64_t>(sensor->Value.GetValueOrDefault());
                    }
                }
                // 无SensorType::Health，无法直接获取健康状态
            }

            // SMART属性
            info.smartAttributes = GetSmartAttributes(info.name);

            // 分区信息无法直接通过LibreHardwareMonitor获取，可通过WMI/WinAPI补充
            // 留空
            result.push_back(info);
        }
    }
    return result;
}

// 获取指定物理磁盘的SMART详细属性
std::vector<SmartAttribute> LibreHardwareMonitorBridge::GetSmartAttributes(const std::string& physicalDiskName) {
    std::vector<SmartAttribute> attrs;
    if (!initialized) return attrs;
    computer->Accept(visitor);

    for each (IHardware ^ hardware in computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Storage &&
            msclr::interop::marshal_as<std::string>(hardware->Name) == physicalDiskName) {
            // 遍历所有Sensors，筛选SMART相关
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Data) {
                    SmartAttribute a;
                    // 尝试从Name中解析ID和名称
                    std::string sname = msclr::interop::marshal_as<std::string>(sensor->Name);
                    // 例： "05 Reallocated Sectors Count"
                    size_t spacePos = sname.find(' ');
                    if (spacePos != std::string::npos) {
                        try {
                            a.id = std::stoi(sname.substr(0, spacePos));
                        } catch (...) {
                            a.id = 0;
                        }
                        a.name = sname.substr(spacePos + 1);
                    } else {
                        a.id = 0;
                        a.name = sname;
                    }
                    a.value = sensor->Value.HasValue ? static_cast<int>(sensor->Value.GetValueOrDefault()) : 0;
                    a.worst = 0;     // LibreHardwareMonitor未直接提供
                    a.threshold = 0; // LibreHardwareMonitor未直接提供
                    a.raw = 0;       // LibreHardwareMonitor未直接提供
                    attrs.push_back(a);
                }
            }
            break;
        }
    }
    return attrs;
}
