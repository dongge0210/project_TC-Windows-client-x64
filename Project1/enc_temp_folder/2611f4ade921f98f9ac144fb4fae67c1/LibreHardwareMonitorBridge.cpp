#ifdef _MSC_VER
#pragma managed(push, off)
#endif

#include "LibreHardwareMonitorBridge.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <Windows.h>
#include <Logger.h>

bool LibreHardwareMonitorBridge::initialized = false;
std::string LibreHardwareMonitorBridge::lastError = "";
std::unique_ptr<LibreHardwareMonitorBridge::CLRBridgeImpl> LibreHardwareMonitorBridge::impl = nullptr;
const char* const LibreHardwareMonitorBridge::DEFAULT_DLL_PATH = "LibreHardwareMonitorLib.dll";


#ifdef _MSC_VER
#pragma managed(pop)
#pragma managed(push, on)

#using "F:\\Win_x64-10.lastest-sysMonitor\\src\\third_party\\LibreHardwareMonitor-0.9.4\\bin\\Debug\\net472\\LibreHardwareMonitorLib.dll"

#include <msclr/marshal_cppstd.h>
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace msclr::interop;

// 实现类定义
class LibreHardwareMonitorBridge::CLRBridgeImpl {
private:
    gcroot<LibreHardwareMonitor::Hardware::Computer^> computer;
    bool isValid = false;

    // Helper to extract detailed exception info
    static std::string GetExceptionDetails(System::Exception^ ex) {
        try {
            System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
            sb->Append("CLR Exception: ");
            if (ex != nullptr) {
                try {
                    sb->Append(ex->ToString());
                }
                catch (...) {
                    sb->Append("Exception.ToString() failed.");
                }
                if (ex->InnerException != nullptr) {
                    sb->Append("\nInnerException: ");
                    try {
                        sb->Append(ex->InnerException->ToString());
                    }
                    catch (...) {
                        sb->Append("InnerException.ToString() failed.");
                    }
                }
            } else {
                sb->Append("Unknown managed exception (null).");
            }
            return marshal_as<std::string>(sb->ToString());
        }
        catch (...) {
            return "CLR Exception: (failed to extract exception details)";
        }
    }

    // Diagnostic logger
    static void LogDiag(const std::string& msg) {
        Logger::Info("[LibreHardwareMonitorBridge::CLRBridgeImpl] " + msg);
    }

public:
    CLRBridgeImpl(const std::string& dllPath) {
        std::string actualPath = dllPath.empty() ? LibreHardwareMonitorBridge::DEFAULT_DLL_PATH : dllPath;
        LogDiag("Requested DLL path: " + dllPath);
        LogDiag("Actual DLL path: " + actualPath);

        // Early check for DLL existence before any managed code
        if (!std::filesystem::exists(actualPath)) {
            LogDiag("DLL does not exist at: " + actualPath);
            std::string errMsg = "LibreHardwareMonitorLib.dll not found at: " + actualPath +
                ". Please ensure the DLL is present in the application directory or the specified path. " 
                "Check your deployment or build output for LibreHardwareMonitorLib.dll.";
            Logger::Error(errMsg);
            throw std::runtime_error(errMsg);
        }
        LogDiag("DLL exists at: " + actualPath);

        try {
            // 加载 LibreHardwareMonitorLib.dll
            LogDiag("Attempting Assembly::LoadFrom...");
            Assembly^ lhmAssembly = nullptr;
            try {
                lhmAssembly = Assembly::LoadFrom(gcnew String(actualPath.c_str()));
            } catch (System::Exception^ ex) {
                LogDiag("Assembly::LoadFrom threw: " + GetExceptionDetails(ex));
                throw;
            }
            if (!lhmAssembly) {
                LogDiag("Assembly::LoadFrom returned nullptr.");
                throw std::runtime_error("无法加载 LibreHardwareMonitorLib.dll");
            }
            LogDiag("Assembly loaded successfully.");

            LogDiag("Creating LibreHardwareMonitor::Hardware::Computer...");
            computer = gcnew LibreHardwareMonitor::Hardware::Computer();
            if (!computer) {
                LogDiag("Failed to create Computer object.");
                throw std::runtime_error("Failed to create Computer object.");
            }
            LogDiag("Computer object created.");

            computer->IsCpuEnabled = true;
            computer->IsGpuEnabled = true;
            computer->IsMotherboardEnabled = true;
            computer->IsMemoryEnabled = true;
            computer->IsControllerEnabled = true;
            computer->IsStorageEnabled = true;
            computer->IsNetworkEnabled = false;

            LogDiag("Opening Computer...");
            computer->Open();
            LogDiag("Computer opened.");

            isValid = true;
            LogDiag("CLRBridgeImpl initialization successful.");
        }
        catch (System::Runtime::InteropServices::SEHException^ sehEx) {
            std::string msg = "SEHException caught during initialization. This may indicate a .NET/native interop issue, missing dependency, or architecture mismatch.\n";
            msg += GetExceptionDetails(sehEx);
            Logger::Error(msg);
            throw std::runtime_error(msg);
        }
        catch (System::Exception^ ex) {
            std::string msg = GetExceptionDetails(ex);
            Logger::Error("Managed exception during initialization: " + msg);
            throw std::runtime_error(msg);
        }
        catch (const std::exception& ex) {
            Logger::Error(std::string("std::exception during initialization: ") + ex.what());
            throw; // rethrow std exceptions as-is
        }
        catch (...) {
            Logger::Error("Unknown exception during CLRBridgeImpl initialization. Possible causes: .NET runtime mismatch, missing dependencies, or architecture mismatch.");
            throw std::runtime_error("Unknown CLR exception during initialization.");
        }
    }

    ~CLRBridgeImpl() {
        try {
            if (computer) computer->Close();
        }
        catch (System::Exception^) {}
    }

    std::vector<std::pair<std::string, double>> GetTemperatures() {
        std::vector<std::pair<std::string, double>> temps;
        if (!isValid) return temps;
        try {
            for each (LibreHardwareMonitor::Hardware::IHardware ^ hardware in computer->Hardware) {
                hardware->Update();
                for each (LibreHardwareMonitor::Hardware::ISensor ^ sensor in hardware->Sensors) {
                    if (sensor->SensorType == LibreHardwareMonitor::Hardware::SensorType::Temperature && sensor->Value.HasValue) {
                        std::string name = marshal_as<std::string>(hardware->Name) + " - " + marshal_as<std::string>(sensor->Name);
                        temps.push_back(std::make_pair(name, static_cast<double>(sensor->Value.Value)));
                    }
                }
            }
        }
        catch (System::Exception^ ex) {
            std::string msg = GetExceptionDetails(ex);
            Logger::Error("Error getting temperatures: " + msg);
            std::cerr << "Error getting temperatures: " << msg << std::endl;
        }
        return temps;
    }

    std::string GetRuntimeVersion() const {
        try {
            System::String^ version = System::Environment::Version->ToString();
            return marshal_as<std::string>(version);
        }
        catch (System::Exception^) {
            return "Unknown";
        }
    }

    std::string GetVersion() const {
        return GetRuntimeVersion();
    }
};

#pragma managed(pop)
#else
// 非托管环境下的空实现
class LibreHardwareMonitorBridge::CLRBridgeImpl {
public:
    CLRBridgeImpl(const std::string&) {}
    ~CLRBridgeImpl() {}
    std::vector<std::pair<std::string, double>> GetTemperatures() { return {}; }
    std::string GetRuntimeVersion() const { return "Not available (non-CLR build)"; }
    std::string GetVersion() const { return "Not available (non-CLR build)"; }
};
#endif

// 共用部分保持不变
void LibreHardwareMonitorBridge::Initialize(const std::string& dllPath) {
    if (initialized) return;
    try {
        impl = std::make_unique<CLRBridgeImpl>(dllPath);
        initialized = true;
        lastError = "";
    }
    catch (const std::exception& ex) {
        initialized = false;
        lastError = std::string("LibreHardwareMonitor 初始化失败: ") + ex.what();
        Logger::Warning(lastError);
        std::cerr << "LibreHardwareMonitorBridge initialization failed: " << lastError << std::endl;
    }
}

void LibreHardwareMonitorBridge::Cleanup() {
    impl.reset();
    initialized = false;
}

std::vector<std::pair<std::string, double>> LibreHardwareMonitorBridge::GetTemperatures() {
    if (!initialized || !impl) return {};
    return impl->GetTemperatures();
}

std::string LibreHardwareMonitorBridge::GetDotNetRuntimeVersion() {
    if (!impl) return "Unknown (.NET runtime not initialized)";
    return impl->GetVersion();
}
