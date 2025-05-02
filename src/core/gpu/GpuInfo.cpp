#include "GpuInfo.h"
#include "Logger.h"
#include "WmiManager.h"
#include <comutil.h>
#include <nvml.h>
#include <sstream>
#include <iomanip>

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
        Logger::Info("GPU 信息检测完成");
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
            data.dedicatedMemory = static_cast<uint64_t>(vtAdapterRAM.uintVal);
        }

        if (SUCCEEDED(pclsObj->Get(L"CurrentClockSpeed", 0, &vtCurrentClockSpeed, 0, 0)) && vtCurrentClockSpeed.vt == VT_UI4) {
            data.coreClock = static_cast<double>(vtCurrentClockSpeed.uintVal) / 1e6;
        }

        bool isVirtual = (
            data.name.find(L"Todesk Virtual Display Adapter") != std::wstring::npos ||
            data.name.find(L"Microsoft Basic Display Adapter") != std::wstring::npos
        );

        if (!isVirtual) {
            data.isNvidia = (data.name.find(L"NVIDIA") != std::wstring::npos);
            data.isAmd = (data.name.find(L"AMD") != std::wstring::npos ||
                          data.name.find(L"Radeon") != std::wstring::npos);
            data.isIntegrated = (data.deviceId.find(L"VEN_8086") != std::wstring::npos);
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
            Logger::Info("GPU 检测结果: 发现 " + std::to_string(gpuList.size()) + " 个 GPU");

            // 输出检测到的 GPU 信息
            for (size_t i = 0; i < gpuList.size(); ++i) {
                const auto& gpu = gpuList[i];
                std::wstring wname = gpu.name;
                std::string name(wname.begin(), wname.end());

                std::string type;
                if (gpu.isNvidia) type = "NVIDIA";
                else if (gpu.isAmd) type = "AMD";
                else if (gpu.isIntegrated) type = "Intel 集成显卡";
                else type = "其他";

                std::string memoryInfo;
                if (gpu.dedicatedMemory > 0) {
                    double gbMem = static_cast<double>(gpu.dedicatedMemory) / (1024 * 1024 * 1024);
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(1) << gbMem;
                    memoryInfo = ss.str() + " GB";
                } else {
                    memoryInfo = "未知";
                }

                Logger::Info("GPU #" + std::to_string(i + 1) + ": " + name + " (" + type + ", " +
                             memoryInfo + " 显存)");
            }
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
                gpuList[index].dedicatedMemory = desc.DedicatedVideoMemory;
                Logger::Info("已获取 Intel GPU 显存大小: " +
                             std::to_string(desc.DedicatedVideoMemory / (1024 * 1024)) + " MB");
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
            gpuList[index].dedicatedMemory = memory.total;
            if (!memoryLogged) {
                Logger::Info("NVIDIA GPU 显存: " + std::to_string(memory.total / (1024 * 1024)) + " MB");
                memoryLogged = true;
            }
        }

        // 获取核心频率
        unsigned int clockMHz = 0;
        result = nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &clockMHz);
        if (NVML_SUCCESS == result) {
            gpuList[index].coreClock = static_cast<double>(clockMHz);
            Logger::Info("NVIDIA GPU 核心频率: " + std::to_string(clockMHz) + " MHz");
        }

        // 获取计算能力
        int major, minor;
        result = nvmlDeviceGetCudaComputeCapability(device, &major, &minor);
        if (NVML_SUCCESS == result) {
            gpuList[index].computeCapabilityMajor = major;
            gpuList[index].computeCapabilityMinor = minor;
            if (!capabilityLogged) {
                Logger::Info("NVIDIA GPU 计算能力: " + std::to_string(major) + "." + std::to_string(minor));
                capabilityLogged = true;
            }
        }
        // 保留 NVML 资源以便后续使用

    } catch (const std::exception& e) {
        Logger::Error("查询 NVIDIA GPU 信息时发生异常: " + std::string(e.what()));
    } catch (...) {
        Logger::Error("查询 NVIDIA GPU 信息时发生未知异常");
    }
}

const std::vector<GpuInfo::GpuData>& GpuInfo::GetGpuData() const {
    return gpuList;
}