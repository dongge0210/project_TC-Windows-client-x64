#include "GpuInfo.h"
#include "../Utils/Logger.h"
#include "WmiManager.h"
#include <comutil.h>
#include <nvml.h>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <limits>
#include <Wbemidl.h>

// 保证没有与comutil.h冲突的全局符号、宏、using等
// 不要定义Data_t、operator=、operator+等与comutil.h同名的内容

// 避免重复日志的静态变量
static bool firstGpuDetection = true;

GpuInfo::GpuInfo(WmiManager& manager) : wmiManager(manager) {
    if (!wmiManager.IsInitialized()) {
        Logger::Error("WMI服务未初始化");
        return;
    }
    pSvc = wmiManager.GetWmiService();
    DetectGpusViaWmi();
}

GpuInfo::~GpuInfo() {
    // 仅在第一次输出 GPU 完成日志
    if (firstGpuDetection) {
        firstGpuDetection = false;
    }
}

void GpuInfo::DetectGpusViaWmi() {
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hres)) {
        Logger::Error("WMI 查询 Win32_VideoController 失败，错误码: 0x" +
                      std::string(std::to_string(hres)));
        return;
    }

    ULONG uReturn = 0;
    IWbemClassObject* pclsObj = nullptr;
    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
        GpuData data;
        VARIANT vtName, vtPnpId, vtAdapterRAM, vtCurrentClockSpeed;
        VariantInit(&vtName);
        VariantInit(&vtPnpId);
        VariantInit(&vtAdapterRAM);
        VariantInit(&vtCurrentClockSpeed);

        if (SUCCEEDED(pclsObj->Get(L"Name", 0, &vtName, 0, 0)) && vtName.vt == VT_BSTR) {
            data.name = vtName.bstrVal;
        }

        if (SUCCEEDED(pclsObj->Get(L"PNPDeviceID", 0, &vtPnpId, 0, 0)) && vtPnpId.vt == VT_BSTR) {
            data.deviceId = vtPnpId.bstrVal;
        }

        if (SUCCEEDED(pclsObj->Get(L"AdapterRAM", 0, &vtAdapterRAM, 0, 0)) && vtAdapterRAM.vt == VT_UI4) {
            data.vram = static_cast<uint64_t>(vtAdapterRAM.uintVal);
        } else {
            data.vram = 0; // 明确为0
        }

        // 新增：尝试获取共享内存
        // 这里可以通过DXGI或WMI进一步完善
        data.sharedMemory = 0; // 若后续能获取则赋值

        if (SUCCEEDED(pclsObj->Get(L"CurrentClockSpeed", 0, &vtCurrentClockSpeed, 0, 0)) && vtCurrentClockSpeed.vt == VT_UI4) {
            data.coreClock = static_cast<double>(vtCurrentClockSpeed.uintVal) / 1e6;
        }

        // Expand virtual GPU detection with more keywords (add your screenshot's name if needed)
        bool isVirtual = (
            data.name.find(L"Todesk Virtual Display Adapter") != std::wstring::npos ||
            data.name.find(L"Microsoft Basic Display Adapter") != std::wstring::npos ||
            data.name.find(L"VMware") != std::wstring::npos ||
            data.name.find(L"VirtualBox") != std::wstring::npos ||
            data.name.find(L"VBox") != std::wstring::npos ||
            data.name.find(L"Parallels") != std::wstring::npos ||
            data.name.find(L"QEMU") != std::wstring::npos ||
            data.name.find(L"Virtual GPU") != std::wstring::npos ||
            data.name.find(L"Citrix") != std::wstring::npos ||
            data.name.find(L"Hyper-V") != std::wstring::npos ||
            data.name.find(L"VIRTIO") != std::wstring::npos ||
            data.name.find(L"Basic Display") != std::wstring::npos ||
            data.name.find(L"AskLinkIddDriver Device") != std::wstring::npos ||
            data.name.find(L"Microsoft Remote Display Adapter") != std::wstring::npos
        );

        if (isVirtual) {
            // Log skipped virtual GPU for debugging
            std::wstring wname = data.name;
            std::string name(wname.begin(), wname.end());
            Logger::Info("检测到虚拟显卡，已跳过: " + name);
        } else {
            data.isNvidia = (data.name.find(L"NVIDIA") != std::wstring::npos);
            data.isAmd = (data.name.find(L"AMD") != std::wstring::npos ||
                          data.name.find(L"Radeon") != std::wstring::npos);
            data.isIntegrated = (data.deviceId.find(L"VEN_8086") != std::wstring::npos);

            // 新增：查询 Win32_PnPSignedDriver 获取驱动信息
            IEnumWbemClassObject* pDrvEnum = nullptr;
            std::wstring wql = L"SELECT * FROM Win32_PnPSignedDriver WHERE DeviceID='" + data.deviceId + L"'";
            HRESULT drvRes = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t(wql.c_str()),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                nullptr,
                &pDrvEnum
            );
            if (SUCCEEDED(drvRes) && pDrvEnum) {
                IWbemClassObject* pDrvObj = nullptr;
                ULONG drvRet = 0;
                if (pDrvEnum->Next(WBEM_INFINITE, 1, &pDrvObj, &drvRet) == S_OK) {
                    VARIANT vtDrvVer, vtDrvDate, vtDrvProvider;
                    VariantInit(&vtDrvVer);
                    VariantInit(&vtDrvDate);
                    VariantInit(&vtDrvProvider);
                    if (SUCCEEDED(pDrvObj->Get(L"DriverVersion", 0, &vtDrvVer, 0, 0)) && vtDrvVer.vt == VT_BSTR)
                        data.driverVersion = vtDrvVer.bstrVal;
                    if (SUCCEEDED(pDrvObj->Get(L"DriverDate", 0, &vtDrvDate, 0, 0)) && vtDrvDate.vt == VT_BSTR)
                        data.driverDate = vtDrvDate.bstrVal;
                    if (SUCCEEDED(pDrvObj->Get(L"DriverProviderName", 0, &vtDrvProvider, 0, 0)) && vtDrvProvider.vt == VT_BSTR)
                        data.driverProvider = vtDrvProvider.bstrVal;
                    VariantClear(&vtDrvVer);
                    VariantClear(&vtDrvDate);
                    VariantClear(&vtDrvProvider);
                    pDrvObj->Release();
                }
                pDrvEnum->Release();
            }

            gpuList.push_back(data);
        }

        VariantClear(&vtName);
        VariantClear(&vtPnpId);
        VariantClear(&vtAdapterRAM);
        VariantClear(&vtCurrentClockSpeed);
        pclsObj->Release();
    }

    pEnumerator->Release();

    if (gpuList.empty()) {
        Logger::Warning("未检测到有效的 GPU。");
    } else {
        // 只在首次检测时输出日志
        if (firstGpuDetection) {
            // Logger::Info("GPU 检测结果: 发现 " + std::to_string(gpuList.size()) + " 个 GPU");

            // 输出检测到的 GPU 信息
            // for (size_t i = 0; i < gpuList.size(); ++i) {
            //     const auto& gpu = gpuList[i];
            //     std::wstring wname = gpu.name;
            //     std::string name(wname.begin(), wname.end());

            //     std::string type;
            //     if (gpu.isNvidia) type = "NVIDIA";
            //     else if (gpu.isAmd) type = "AMD";
            //     else if (gpu.isIntegrated) type = "Intel 集成显卡";
            //     else type = "其他";

            //     std::string memoryInfo;
            //     if (gpu.dedicatedMemory > 0) {
            //         double gbMem = static_cast<double>(gpu.dedicatedMemory) / (1024 * 1024 * 1024);
            //         std::stringstream ss;
            //         ss << std::fixed << std::setprecision(1) << gbMem;
            //         memoryInfo = ss.str() + " GB";
            //     } else {
            //         memoryInfo = "未知";
            //     }

            //     Logger::Warning("GPU #" + std::to_string(i + 1) + ": " + name + " (" + type + ", " +
            //                     memoryInfo + " 显存)");
            // }
        }
    }

    // 查询特定型号 GPU 的额外信息
    for (size_t i = 0; i < gpuList.size(); ++i) {
        if (gpuList[i].isNvidia) {
            QueryNvidiaGpuInfo(static_cast<int>(i));
        } else if (gpuList[i].isIntegrated) {
            QueryIntelGpuInfo(static_cast<int>(i));
        }
    }
}

void GpuInfo::QueryIntelGpuInfo(int index) {
    if (index < 0 || index >= static_cast<int>(gpuList.size())) {
        Logger::Error("尝试查询无效的 Intel GPU 索引: " + std::to_string(index));
        return;
    }

    try {
        IDXGIFactory* pFactory = nullptr;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory))) {
            Logger::Error("无法创建 DXGI 工厂，无法获取 Intel GPU 详细信息");
            return;
        }

        IDXGIAdapter* pAdapter = nullptr;
        if (SUCCEEDED(pFactory->EnumAdapters(index, &pAdapter))) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(pAdapter->GetDesc(&desc))) {
                gpuList[index].vram = desc.DedicatedVideoMemory;  // 专用显存
                gpuList[index].sharedMemory = desc.SharedSystemMemory; // 共享系统内存
                Logger::Info("Intel GPU 专用显存: " + std::to_string(desc.DedicatedVideoMemory / (1024 * 1024)) + " MB");
                Logger::Info("Intel GPU 共享内存: " + std::to_string(desc.SharedSystemMemory / (1024 * 1024)) + " MB");
            }
            pAdapter->Release();
        }
        pFactory->Release();
    } catch (const std::exception& e) {
        Logger::Error("查询 Intel GPU 信息时发生异常: " + std::string(e.what()));
    } catch (...) {
        Logger::Error("查询 Intel GPU 信息时发生未知异常");
    }
}

void GpuInfo::QueryNvidiaGpuInfo(int index) {
    // 提前检查索引合法性
    if (index < 0 || index >= static_cast<int>(gpuList.size())) {
        Logger::Error("尝试查询无效的 NVIDIA GPU 索引: " + std::to_string(index));
        return;
    }

    try {
        static bool nvmlInitialized = false;
        // 合并显存与计算能力日志的标记，避免重复打印
        static bool memoryLogged = false;
        static bool capabilityLogged = false;

        // 避免重复初始化 NVML
        if (!nvmlInitialized) {
            nvmlReturn_t initResult = nvmlInit();
            if (NVML_SUCCESS != initResult) {
                Logger::Error("NVML 初始化失败: " + std::string(nvmlErrorString(initResult)));
                return;
            }
            nvmlInitialized = true;
        }

        // 设备句柄获取
        nvmlDevice_t device;
        nvmlReturn_t result = nvmlDeviceGetHandleByIndex(0, &device);
        if (NVML_SUCCESS != result) {
            Logger::Error("获取 NVIDIA 设备句柄失败: " + std::string(nvmlErrorString(result)));
            return;
        }

        // 获取显存信息
        nvmlMemory_t memory;
        result = nvmlDeviceGetMemoryInfo(device, &memory);
        if (NVML_SUCCESS == result) {
            gpuList[index].vram = memory.total;     // 将 NVML 获取的显存赋值给 vram
        }

        // 获取核心频率
        unsigned int clockMHz = 0;
        result = nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &clockMHz);
        if (NVML_SUCCESS == result) {
            gpuList[index].coreClock = static_cast<double>(clockMHz);
        }

        // 获取计算能力
        int major, minor;
        result = nvmlDeviceGetCudaComputeCapability(device, &major, &minor);
        if (NVML_SUCCESS == result) {
            gpuList[index].computeCapabilityMajor = major;
            gpuList[index].computeCapabilityMinor = minor;
        }

        // 获取驱动版本（NVML）
        char driverVersion[80] = {0};
        result = nvmlSystemGetDriverVersion(driverVersion, sizeof(driverVersion));
        if (NVML_SUCCESS == result) {
            // 转换为wstring
            std::string drvStr(driverVersion);
            std::wstring drvWstr(drvStr.begin(), drvStr.end());
            gpuList[index].driverVersion = drvWstr;
        }

        // GPU功率日志输出，单位为W，不带百分号
        unsigned int power_mw = 0;
        if (nvmlDeviceGetPowerUsage(device, &power_mw) == NVML_SUCCESS) {
            double power = power_mw / 1000.0; // 转换为瓦特
            Logger::Info("GPU功率: " + std::to_string(power) + " W");
        }
        // 保留 NVML 资源以便后续使用

    }
    catch (const std::exception& e) {
        Logger::Error("查询 NVIDIA GPU 信息时发生异常: " + std::string(e.what()));
    }
    catch (...) {
        Logger::Error("查询 NVIDIA GPU 信息时发生未知异常");
    }
}

double GpuInfo::GetGpuPowerNVML() {
#ifdef _WIN32
    static bool loggedNotSupported = false; // 防止重复刷屏
    nvmlReturn_t result;
    result = nvmlInit_v2();
    if (result != NVML_SUCCESS) {
        Logger::Error(std::string("NVML 初始化失败: ") + nvmlErrorString(result));
        return std::numeric_limits<double>::quiet_NaN();
    }

    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex_v2(0, &device); // 取第一个GPU
    if (result != NVML_SUCCESS) {
        Logger::Error(std::string("NVML 获取设备句柄失败: ") + nvmlErrorString(result));
        nvmlShutdown();
        return std::numeric_limits<double>::quiet_NaN();
    }

    nvmlEnableState_t pmSupported = NVML_FEATURE_DISABLED;
    result = nvmlDeviceGetPowerManagementMode(device, &pmSupported);
    if (result != NVML_SUCCESS) {
        Logger::Error(std::string("NVML 检查功率管理支持失败: ") + nvmlErrorString(result));
        nvmlShutdown();
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (pmSupported == NVML_FEATURE_DISABLED) {
        char name[NVML_DEVICE_NAME_BUFFER_SIZE] = {0};
        nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
        if (!loggedNotSupported) {
            Logger::Warning(std::string("NVML: 设备 [") + name + "] 不支持功率监控，跳过功率读取。");
            loggedNotSupported = true;
        }
        nvmlShutdown();
        return std::numeric_limits<double>::quiet_NaN();
    }

    unsigned int power_mw = 0;
    result = nvmlDeviceGetPowerUsage(device, &power_mw);
    if (result != NVML_SUCCESS) {
        if (!loggedNotSupported) {
            Logger::Warning("NVML: 设备支持功率管理，但获取GPU功率失败: " + std::string(nvmlErrorString(result)));
            loggedNotSupported = true;
        }
        nvmlShutdown();
        return std::numeric_limits<double>::quiet_NaN();
    }
    nvmlShutdown();

    if (power_mw == 0) {
        if (!loggedNotSupported) {
            Logger::Info("NVML 获取到的GPU功率为0，可能未支持或无效。");
            loggedNotSupported = true;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }

    double power = static_cast<double>(power_mw) / 1000.0;
    Logger::Info("NVML GPU功率: " + std::to_string(power) + " W");
    return power;
#else
    return std::numeric_limits<double>::quiet_NaN();
#endif
}

const std::vector<GpuInfo::GpuData>& GpuInfo::GetGpuData() const {
    return gpuList;
}