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

void LibreHardwareMonitorBridge::Initialize() {
    try {
        if (initialized) return;
        computer = gcnew Computer();
        computer->IsCpuEnabled = true;
        computer->IsGpuEnabled = true;
        computer->Open();
        visitor = gcnew ::UpdateVisitor();
        initialized = true;
        Logger::Info("LibreHardwareMonitor 初始化完成");
    }
    catch (System::IO::FileNotFoundException^ ex) {
        std::string errorMsg = msclr::interop::marshal_as<std::string>(ex->Message);
        Logger::Error("LibreHardwareMonitor 初始化失败: " + errorMsg);
    }
    catch (System::Exception^ ex) {
        std::string errorMsg = msclr::interop::marshal_as<std::string>(ex->Message);
        Logger::Error("LibreHardwareMonitor 初始化异常: " + errorMsg);
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
    for each (IHardware ^ hardware in computer->Hardware) {
        hardware->Update();
        std::string hwName = msclr::interop::marshal_as<std::string>(hardware->Name);
    
      if (hardware->HardwareType == HardwareType::Cpu) {
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature && sensor->Value.HasValue) {
                    std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                    // 只采集"CPU Package"或"Core #"开头的温度
                    if (name == "CPU Package" || name.find("Core #") == 0) {
                        double value = sensor->Value.GetValueOrDefault();
                        temps.push_back({ name, value });
                        Logger::Info("温度传感器: " + name + " = " + std::to_string(value));
                    }
                }
            }
        }

        // 只采集每个GPU的GPU Core温度，并带序号
        else if (hardware->HardwareType == HardwareType::GpuNvidia ||
            hardware->HardwareType == HardwareType::GpuAmd ||
            hardware->HardwareType == HardwareType::GpuIntel) {
            for each (ISensor ^ sensor in hardware->Sensors) {
                if (sensor->SensorType == SensorType::Temperature && sensor->Value.HasValue) {
                    std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                    if (name == "GPU Core") {
                        double value = sensor->Value.GetValueOrDefault();
                        std::string gpuLabel = "GPU#" + std::to_string(gpuIndex) + " Core";
                        temps.push_back({ gpuLabel, value });
                        Logger::Info("温度传感器: " + gpuLabel + " = " + std::to_string(value));
                    }
                    if (hardware->HardwareType == HardwareType::Cpu) {
                        for each (ISensor ^ sensor in hardware->Sensors) {
                            std::string name = msclr::interop::marshal_as<std::string>(sensor->Name);
                            Logger::Info("CPU传感器: " + name);
                        }
                    }
                }
            }
            gpuIndex++;
        }
    }
    Logger::Info("采集到温度传感器数量: " + std::to_string(temps.size()));
    return temps;
}

double LibreHardwareMonitorBridge::GetCpuPower() {
    // 实现：从监控数据中获取CPU功率，单位瓦特
    double cpuPower = 0.0; // TODO: 实现实际获取
    Logger::Info("CPU功率: " + std::to_string(cpuPower) + " W");
    return cpuPower;
}

double LibreHardwareMonitorBridge::GetGpuPower() {
    // 实现：从监控数据中获取GPU功率，单位瓦特
    double gpuPower = 0.0; // TODO: 实现实际获取
    Logger::Info("GPU功率: " + std::to_string(gpuPower) + " W");
    return gpuPower;
}

double LibreHardwareMonitorBridge::GetTotalPower() {
    // 实现：从监控数据中获取整机功率，单位瓦特
    double totalPower = 0.0; // TODO: 实现实际获取
    Logger::Info("整机功率: " + std::to_string(totalPower) + " W");
    return totalPower;
}

