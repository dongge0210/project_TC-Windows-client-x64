#include "LibreHardwareMonitorBridge.h"
#include "Logger.h"
#include <msclr/marshal_cppstd.h>
#include <iostream>
#include <windows.h>
#using "F:\\Win_x64-10.lastest-sysMonitor\\src\\third_party\\LibreHardwareMonitor-0.9.4\\bin\\Debug\\net472\\LibreHardwareMonitorLib.dll"

using namespace LibreHardwareMonitor::Hardware;
using namespace System;
using namespace System::Collections::Generic;
using namespace msclr::interop;

// 定义静态成员
bool LibreHardwareMonitorBridge::initialized = false;
msclr::gcroot<Computer^> LibreHardwareMonitorBridge::computer;
msclr::gcroot<IVisitor^> LibreHardwareMonitorBridge::visitor;

public ref class UpdateVisitor : public IVisitor
{
public:
    virtual void VisitComputer(IComputer^ computer) override {}
    virtual void VisitHardware(IHardware^ hardware) override
    {
        hardware->Update();
        for each (IHardware ^ subHardware in hardware->SubHardware)
            subHardware->Accept(this);
    }
    virtual void VisitSensor(ISensor^ sensor) override {}
    virtual void VisitParameter(IParameter^ parameter) override {}
};

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

        // 新增：输出所有硬件类型和名称
        for each (IHardware ^ hardware in computer->Hardware) {
            std::string hwType = msclr::interop::marshal_as<std::string>(hardware->HardwareType.ToString());
            std::string hwName = msclr::interop::marshal_as<std::string>(hardware->Name);
            Logger::Info("[调试] 枚举到硬件: 类型=" + hwType + ", 名称=" + hwName);
        }

        // 新增：初始化后立即诊断所有硬件和传感器
        DiagnoseAllHardwareAndSensors();
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
}

std::vector<std::pair<std::string, double>> LibreHardwareMonitorBridge::GetTemperatures() {
    std::vector<std::pair<std::string, double>> temps;
    if (!initialized) return temps;

    computer->Accept(visitor);

    int gpuIndex = 0;
    int tempSensorCount = 0;
    int invalidTempCount = 0;
    int missingSensorCount = 0;

    // Enhanced debug: log all sensors and their HasValue status
    for each (IHardware ^ hardware in computer->Hardware) {
        hardware->Update();
        std::string hwName = msclr::interop::marshal_as<std::string>(hardware->Name);

        // Log all sensors for this hardware (for debugging)
        for each (ISensor ^ sensor in hardware->Sensors) {
            std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
            std::string type = msclr::interop::marshal_as<std::string>(sensor->SensorType.ToString());
            bool hasValue = sensor->Value.HasValue;
            double value = hasValue ? sensor->Value.GetValueOrDefault() : -9999.0;
            std::string identifier = msclr::interop::marshal_as<std::string>(sensor->Identifier->ToString());
            Logger::Info("[调试] 传感器: " + name + ", 类型: " + type + ", HasValue: " + (hasValue ? "true" : "false") + ", Value: " + std::to_string(value) + ", Identifier: " + identifier);
        }

        // Collect all temperature sensors for CPU
        if (hardware->HardwareType == HardwareType::Cpu) {
            bool foundTempSensor = false;
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature) {
                    foundTempSensor = true;
                    std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                    double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : -9999.0;
                    temps.push_back({ name, value });
                    tempSensorCount++;
                    if (!sensor->Value.HasValue) invalidTempCount++;
                    if (sensor->Value.HasValue) {
                        Logger::Info("有效温度: " + name + " = " + std::to_string(value));
                    } else {
                        Logger::Warning("[无效] 温度传感器: " + name + ", Value: -9999.0 (HasValue == false)");
                    }
                }
            }
            if (!foundTempSensor) {
                Logger::Warning("[诊断] 未发现CPU温度传感器: " + hwName);
                missingSensorCount++;
            }
        }
        // Collect all temperature sensors for GPU
        else if (hardware->HardwareType == HardwareType::GpuNvidia ||
            hardware->HardwareType == HardwareType::GpuAmd ||
            hardware->HardwareType == HardwareType::GpuIntel) {
            bool foundTempSensor = false;
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature) {
                    foundTempSensor = true;
                    std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                    double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : -9999.0;
                    std::string gpuLabel = "GPU#" + std::to_string(gpuIndex) + "/" + name;
                    temps.push_back({ gpuLabel, value });
                    tempSensorCount++;
                    if (!sensor->Value.HasValue) invalidTempCount++;
                    if (sensor->Value.HasValue) {
                        Logger::Info("有效温度: " + gpuLabel + " = " + std::to_string(value));
                    } else {
                        Logger::Warning("[无效] 温度传感器: " + gpuLabel + ", Value: -9999.0 (HasValue == false)");
                    }
                }
            }
            if (!foundTempSensor) {
                Logger::Warning("[诊断] 未发现GPU温度传感器: " + hwName);
                missingSensorCount++;
            }
            gpuIndex++;
        }
    }

    Logger::Info("采集到温度传感器数量: " + std::to_string(temps.size()));

    // If all temperature sensors are invalid, log a warning for easier debugging
    if (tempSensorCount > 0 && invalidTempCount == tempSensorCount) {
        Logger::Warning("[诊断] 所有温度传感器均无效 (HasValue == false)。可能原因：驱动/权限/初始化问题。建议检查：1) 以管理员权限运行；2) 检查目标平台和依赖库；3) 传感器是否被其他软件占用。");
    }
    // If no temperature sensors found at all
    if (tempSensorCount == 0) {
        Logger::Warning("[诊断] 未发现任何温度传感器。请检查硬件支持、驱动、依赖库和权限。");
    }

    return temps;
}

// 递归遍历所有hardware和其subHardware，详细输出所有Power类型传感器信息
void LogAllPowerSensors(IHardware^ hardware, std::string prefix, double& maxValue, std::string& maxName) {
    hardware->Update();
    std::string hardwareType = msclr::interop::marshal_as<std::string>(hardware->HardwareType.ToString());
    std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);
    for each (ISensor ^ sensor in hardware->Sensors) {
        std::string sensorTypeStr = msclr::interop::marshal_as<std::string>(sensor->SensorType.ToString());
        if (sensor->SensorType == SensorType::Power) {
            std::string name = prefix + msclr::interop::marshal_as<std::string>(sensor->Name);
            double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : -1.0;
            bool hasValue = sensor->Value.HasValue;
            std::string identifier = msclr::interop::marshal_as<std::string>(sensor->Identifier->ToString());
            int index = sensor->Index;
            double min = sensor->Min.HasValue ? sensor->Min.Value : 0.0;
            double max = sensor->Max.HasValue ? sensor->Max.Value : 0.0;
            Logger::Info("[调试] Power传感器: " + name +
                         ", SensorType: " + sensorTypeStr +
                         ", 所属硬件类型: " + hardwareType +
                         ", 所属硬件名称: " + hardwareName +
                         ", Identifier: " + identifier +
                         ", Index: " + std::to_string(index) +
                         ", 当前值: " + std::to_string(value) +
                         ", HasValue: " + (hasValue ? "true" : "false") +
                         ", Min: " + std::to_string(min) +
                         ", Max: " + std::to_string(max));
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

// 新增：递归输出所有温度和功率传感器信息（辅助调试）
void LogAllSensors(IHardware^ hardware, std::string prefix = "") {
    hardware->Update();
    std::string hardwareType = msclr::interop::marshal_as<std::string>(hardware->HardwareType.ToString());
    std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);
    for each (ISensor ^ sensor in hardware->Sensors) {
        std::string name = prefix + msclr::interop::marshal_as<std::string>(sensor->Name);
        std::string type = msclr::interop::marshal_as<std::string>(sensor->SensorType.ToString());
        double value = sensor->Value.HasValue ? sensor->Value.GetValueOrDefault() : -1.0;
        bool hasValue = sensor->Value.HasValue;
        std::string identifier = msclr::interop::marshal_as<std::string>(sensor->Identifier->ToString());
        int index = sensor->Index;
        double min = sensor->Min.HasValue ? sensor->Min.Value : 0.0;
        double max = sensor->Max.HasValue ? sensor->Max.Value : 0.0;
        Logger::Info("[调试] 传感器: " + name +
                     ", 类型: " + type +
                     ", 所属硬件类型: " + hardwareType +
                     ", 所属硬件名称: " + hardwareName +
                     ", Identifier: " + identifier +
                     ", Index: " + std::to_string(index) +
                     ", 当前值: " + std::to_string(value) +
                     ", HasValue: " + (hasValue ? "true" : "false") +
                     ", Min: " + std::to_string(min) +
                     ", Max: " + std::to_string(max));
    }
    for each (IHardware ^ sub in hardware->SubHardware) {
        LogAllSensors(sub, prefix + msclr::interop::marshal_as<std::string>(sub->Name) + "/");
    }
}

// 新增：诊断函数实现，递归输出所有硬件和传感器详细信息
void LibreHardwareMonitorBridge::DiagnoseAllHardwareAndSensors() {
    if (!initialized) {
        Logger::Warning(L"[诊断] DiagnoseAllHardwareAndSensors: 尚未初始化，无法诊断。");
        return;
    }
    Logger::Info(L"[诊断] 开始输出所有硬件和传感器详细信息：");
    for each (IHardware ^ hardware in computer->Hardware) {
        LogAllSensors(hardware, "[诊断]/");
    }
    Logger::Info(L"[诊断] 结束输出所有硬件和传感器详细信息。");
}

double LibreHardwareMonitorBridge::GetCpuPower() {
    if (!initialized) return 0.0;
    computer->Accept(visitor);

    double maxCpuPower = 0.0;
    std::string maxName;
    bool anyPowerSensor = false;
    for each (IHardware ^ hardware in computer->Hardware) {
        if (hardware->HardwareType == HardwareType::Cpu) {
            // 输出所有传感器信息，辅助调试
            LogAllSensors(hardware, "[CPU]/");
            LogAllPowerSensors(hardware, "", maxCpuPower, maxName);
            anyPowerSensor = true;
        }
    }
    if (!anyPowerSensor) {
        Logger::Warning("[调试] 未发现任何CPU Power类型传感器");
    }
    Logger::Info("[调试] GetCpuPower 返回值: " + std::to_string(maxCpuPower));
    return maxCpuPower;
}

double LibreHardwareMonitorBridge::GetGpuPower() {
    if (!initialized) return 0.0;
    computer->Accept(visitor);

    double maxGpuPower = 0.0;
    std::string maxName;
    bool anyPowerSensor = false;
    for each (IHardware ^ hardware in computer->Hardware) {
        if (hardware->HardwareType == HardwareType::GpuNvidia ||
            hardware->HardwareType == HardwareType::GpuAmd ||
            hardware->HardwareType == HardwareType::GpuIntel) {
            // 输出所有传感器信息，辅助调试
            LogAllSensors(hardware, "[GPU]/");
            LogAllPowerSensors(hardware, "", maxGpuPower, maxName);
            anyPowerSensor = true;
        }
    }
    if (!anyPowerSensor) {
        Logger::Warning("[调试] 未发现任何GPU Power类型传感器");
    }
    Logger::Info("[调试] GetGpuPower 返回值: " + std::to_string(maxGpuPower));
    return maxGpuPower;
}

double LibreHardwareMonitorBridge::GetTotalPower() {
    if (!initialized) return 0.0;
    computer->Accept(visitor);

    double maxTotalPower = 0.0;
    std::string maxName;
    bool anyPowerSensor = false;
    for each (IHardware ^ hardware in computer->Hardware) {
        // 输出所有传感器信息，辅助调试
        LogAllSensors(hardware, "[ALL]/");
        LogAllPowerSensors(hardware, "", maxTotalPower, maxName);
        anyPowerSensor = true;
    }
    if (!anyPowerSensor) {
        Logger::Warning("[调试] 未发现任何总功率 Power类型传感器");
    }
    Logger::Info("[调试] GetTotalPower 返回值: " + std::to_string(maxTotalPower));
    return maxTotalPower;
}

