#include "TpmInfo.h"
#include "../Utils/Logger.h"
#include "../Utils/WMIManager.h"
#include <comutil.h>
#include <tbs.h>        // TPM Base Services
#include <wbemidl.h>

// 链接TBS库和其他必要的库
#pragma comment(lib, "tbs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")

TpmInfo::TpmInfo(WmiManager& manager) : wmiManager(manager) {
    Logger::Info("开始TPM检测 - 优先使用TBS，WMI作为备份方法");
    
    // 初始化检测方法追踪
    tpmData.detectionMethod = L"未检测到";
    tpmData.wmiDetectionWorked = false;
    tpmData.tbsDetectionWorked = false;
    
    // 步骤1: 优先尝试TBS检测
    Logger::Info("第一步: 尝试TBS检测...");
    try {
        DetectTpmViaTbs();
        if (tpmData.tbsAvailable) {
            tpmData.tbsDetectionWorked = true;
            hasTpm = true;
            Logger::Info("TBS检测成功 - 检测到TPM");
        } else {
            Logger::Info("TBS检测失败 - 将尝试WMI备份方法");
        }
    } catch (const std::exception& e) {
        Logger::Error("TBS TPM检测发生异常: " + std::string(e.what()));
        tpmData.errorMessage = L"TBS检测异常: " + std::wstring(e.what(), e.what() + strlen(e.what()));
    } catch (...) {
        Logger::Error("TBS TPM检测发生未知异常");
        tpmData.errorMessage = L"TBS检测发生未知异常";
    }
    
    // 步骤2: 尝试WMI检测（无论TBS是否成功，都尝试获取更多信息）
    Logger::Info("第二步: 尝试WMI检测获取详细信息...");
    
    if (!wmiManager.IsInitialized()) {
        Logger::Warn("WMI服务未初始化，无法通过WMI获取详细TPM信息");
        if (!tpmData.tbsDetectionWorked) {
            tpmData.errorMessage = L"WMI服务未初始化且TBS检测失败";
        }
    } else {
        pSvc = wmiManager.GetWmiService();
        if (!pSvc) {
            Logger::Warn("无法获取WMI服务，无法通过WMI获取详细TPM信息");
            if (!tpmData.tbsDetectionWorked) {
                tpmData.errorMessage = L"无法获取WMI服务且TBS检测失败";
            }
        } else {
            try {
                DetectTpmViaWmi();
                if (tpmData.wmiDetectionWorked) {
                    Logger::Info("WMI检测成功 - 获取到详细TPM信息");
                    if (!hasTpm) {
                        hasTpm = true; // WMI检测到TPM，即使TBS失败
                    }
                } else {
                    Logger::Info("WMI检测未找到TPM信息");
                }
            } catch (const std::exception& e) {
                Logger::Error("WMI TPM检测发生异常: " + std::string(e.what()));
                if (tpmData.errorMessage.empty()) {
                    tpmData.errorMessage = L"WMI检测异常: " + std::wstring(e.what(), e.what() + strlen(e.what()));
                }
            } catch (...) {
                Logger::Error("WMI TPM检测发生未知异常");
                if (tpmData.errorMessage.empty()) {
                    tpmData.errorMessage = L"WMI检测发生未知异常";
                }
            }
        }
    }
    
    // 步骤3: 确定最终的检测方法和状态
    DetermineDetectionMethod();
    
    // 记录最终检测结果
    if (hasTpm) {
        std::string manufacturerStr(tpmData.manufacturerName.begin(), tpmData.manufacturerName.end());
        std::string versionStr(tpmData.version.begin(), tpmData.version.end());
        std::string statusStr(tpmData.status.begin(), tpmData.status.end());
        std::string methodStr(tpmData.detectionMethod.begin(), tpmData.detectionMethod.end());
        Logger::Info("TPM检测完成 - 检测到TPM: " + manufacturerStr + " v" + versionStr + 
                    " (" + statusStr + ") [检测方法: " + methodStr + "]");
    } else {
        std::string errorStr(tpmData.errorMessage.begin(), tpmData.errorMessage.end());
        Logger::Info("TPM检测完成 - 未检测到TPM: " + errorStr);
    }
}

TpmInfo::~TpmInfo() {
    Logger::Info("TPM信息检测结束");
}

void TpmInfo::DetectTpmViaWmi() {
    if (!pSvc) {
        Logger::Error("WMI服务不可用");
        return;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_Tpm"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hres)) {
        Logger::Warn("WMI查询Win32_Tpm失败，可能系统不支持TPM或未启用");
        return;
    }

    ULONG uReturn = 0;
    IWbemClassObject* pclsObj = nullptr;
    bool foundTpmViaWmi = false;
    
    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
        foundTpmViaWmi = true;
        
        VARIANT vtManufacturerName, vtManufacturerId, vtSpecVersion;
        VARIANT vtIsEnabled, vtIsActivated, vtIsOwned, vtPhysicalPresenceRequired;
        
        VariantInit(&vtManufacturerName);
        VariantInit(&vtManufacturerId);
        VariantInit(&vtSpecVersion);
        VariantInit(&vtIsEnabled);
        VariantInit(&vtIsActivated);
        VariantInit(&vtIsOwned);
        VariantInit(&vtPhysicalPresenceRequired);

        // 获取制造商信息
        if (SUCCEEDED(pclsObj->Get(L"ManufacturerName", 0, &vtManufacturerName, 0, 0)) && 
            vtManufacturerName.vt == VT_BSTR) {
            tpmData.manufacturerName = vtManufacturerName.bstrVal;
        }

        if (SUCCEEDED(pclsObj->Get(L"ManufacturerId", 0, &vtManufacturerId, 0, 0)) && 
            vtManufacturerId.vt == VT_I4) {
            wchar_t manufacturerIdStr[32];
            swprintf_s(manufacturerIdStr, L"0x%08X", vtManufacturerId.intVal);
            tpmData.manufacturerId = manufacturerIdStr;
        }

        // 获取规范版本
        if (SUCCEEDED(pclsObj->Get(L"SpecVersion", 0, &vtSpecVersion, 0, 0)) && 
            vtSpecVersion.vt == VT_BSTR) {
            tpmData.version = vtSpecVersion.bstrVal;
        }

        // 获取状态信息
        if (SUCCEEDED(pclsObj->Get(L"IsEnabled_InitialValue", 0, &vtIsEnabled, 0, 0)) && 
            vtIsEnabled.vt == VT_BOOL) {
            tpmData.isEnabled = (vtIsEnabled.boolVal == VARIANT_TRUE);
        }

        if (SUCCEEDED(pclsObj->Get(L"IsActivated_InitialValue", 0, &vtIsActivated, 0, 0)) && 
            vtIsActivated.vt == VT_BOOL) {
            tpmData.isActivated = (vtIsActivated.boolVal == VARIANT_TRUE);
        }

        if (SUCCEEDED(pclsObj->Get(L"IsOwned_InitialValue", 0, &vtIsOwned, 0, 0)) && 
            vtIsOwned.vt == VT_BOOL) {
            tpmData.isOwned = (vtIsOwned.boolVal == VARIANT_TRUE);
        }

        if (SUCCEEDED(pclsObj->Get(L"PhysicalPresenceRequired", 0, &vtPhysicalPresenceRequired, 0, 0)) && 
            vtPhysicalPresenceRequired.vt == VT_BOOL) {
            tpmData.physicalPresenceRequired = (vtPhysicalPresenceRequired.boolVal == VARIANT_TRUE);
        }

        // 设置整体状态
        tpmData.isReady = tpmData.isEnabled && tpmData.isActivated;
        
        if (tpmData.isReady) {
            if (tpmData.status.empty() || tpmData.status == L"通过TBS检测到") {
                tpmData.status = L"就绪";
            }
        } else if (tpmData.isEnabled && !tpmData.isActivated) {
            if (tpmData.status.empty() || tpmData.status == L"通过TBS检测到") {
                tpmData.status = L"已启用但未激活";
            }
        } else if (!tpmData.isEnabled) {
            if (tpmData.status.empty() || tpmData.status == L"通过TBS检测到") {
                tpmData.status = L"未启用";
            }
        } else {
            if (tpmData.status.empty() || tpmData.status == L"通过TBS检测到") {
                tpmData.status = L"未知状态";
            }
        }

        // 清理变量
        VariantClear(&vtManufacturerName);
        VariantClear(&vtManufacturerId);
        VariantClear(&vtSpecVersion);
        VariantClear(&vtIsEnabled);
        VariantClear(&vtIsActivated);
        VariantClear(&vtIsOwned);
        VariantClear(&vtPhysicalPresenceRequired);
        
        pclsObj->Release();
        
        // 记录TPM信息到日志
        std::string manufacturerStr(tpmData.manufacturerName.begin(), tpmData.manufacturerName.end());
        std::string versionStr(tpmData.version.begin(), tpmData.version.end());
        std::string statusStr(tpmData.status.begin(), tpmData.status.end());
        
        Logger::Info("WMI检测到TPM: " + manufacturerStr + 
                    ", 版本: " + versionStr + 
                    ", 状态: " + statusStr + 
                    ", 已启用: " + (tpmData.isEnabled ? "是" : "否") +
                    ", 已激活: " + (tpmData.isActivated ? "是" : "否"));
        
        break; // 只处理第一个TPM
    }

    pEnumerator->Release();

    if (foundTpmViaWmi) {
        tpmData.wmiDetectionWorked = true;
        hasTpm = true;
        Logger::Info("WMI成功检测到TPM设备");
    } else {
        Logger::Info("通过WMI未检测到TPM设备");
    }
}

void TpmInfo::DetectTpmViaTbs() {
    // 尝试检测TBS (TPM Base Services)
    TBS_HCONTEXT hContext = 0;
    
    // 使用TBS_CONTEXT_PARAMS 兼容旧版本Windows SDK
    TBS_CONTEXT_PARAMS contextParams = { 0 };
    contextParams.version = TBS_CONTEXT_VERSION_ONE; // 基础版本，兼容性更好

    TBS_RESULT result = Tbsi_Context_Create(&contextParams, &hContext);
    
    if (result == TBS_SUCCESS) {
        tpmData.tbsAvailable = true;
        
        // 获取TBS版本信息
        TPM_DEVICE_INFO deviceInfo = {0};
        
        result = Tbsi_GetDeviceInfo(sizeof(deviceInfo), &deviceInfo);
        if (result == TBS_SUCCESS) {
            tpmData.tbsVersion = deviceInfo.tpmVersion;
            
            // 根据TPM版本设置版本字符串
            if (deviceInfo.tpmVersion == TPM_VERSION_12) {
                if (tpmData.version.empty()) {
                    tpmData.version = L"1.2";
                }
                Logger::Info("检测到TPM 1.2");
            } else if (deviceInfo.tpmVersion == TPM_VERSION_20) {
                if (tpmData.version.empty()) {
                    tpmData.version = L"2.0";
                }
                Logger::Info("检测到TPM 2.0");
            } else {
                Logger::Info("检测到未知TPM版本: " + std::to_string(deviceInfo.tpmVersion));
            }
            
            Logger::Info("TBS可用, TPM版本: " + std::to_string(deviceInfo.tpmVersion));
        } else {
            Logger::Warn("TBS设备信息获取失败: 0x" + std::to_string(result));
        }
        
        // 如果WMI没有检测到TPM，但TBS可用，说明TPM存在
        if (!hasTpm) {
            hasTpm = true;
            tpmData.isEnabled = true; // TBS可用说明TPM已启用
            tpmData.isActivated = true; // 如果TBS可用，TPM通常也是激活的
            tpmData.status = L"通过TBS检测到";
            Logger::Info("通过TBS检测到TPM");
        }
        
        // 关闭TBS上下文
        Tbsip_Context_Close(hContext);
    } else {
        tpmData.tbsAvailable = false;
        
        // 设置详细的错误信息
        switch (result) {
            case TBS_E_TPM_NOT_FOUND:
                tpmData.errorMessage = L"未找到TPM设备";
                Logger::Info("TBS检测结果: 未找到TPM设备");
                break;
            case TBS_E_SERVICE_NOT_RUNNING:
                tpmData.errorMessage = L"TPM基础服务未运行";
                Logger::Warn("TBS检测结果: TPM基础服务未运行");
                break;
            case TBS_E_INSUFFICIENT_BUFFER:
                tpmData.errorMessage = L"缓冲区不足";
                Logger::Warn("TBS检测结果: 缓冲区不足");
                break;
            case TBS_E_INVALID_PARAMETER:
                tpmData.errorMessage = L"无效参数";
                Logger::Warn("TBS检测结果: 无效参数");
                break;
            case TBS_E_ACCESS_DENIED:
                tpmData.errorMessage = L"访问被拒绝";
                Logger::Warn("TBS检测结果: 访问被拒绝");
                break;
            default:
                tpmData.errorMessage = L"TBS初始化失败: 0x" + std::to_wstring(result);
                Logger::Warn("TBS检测失败，错误代码: 0x" + std::to_string(result));
                break;
        }
        
        std::string errorStr(tpmData.errorMessage.begin(), tpmData.errorMessage.end());
        Logger::Warn("TBS检测失败: " + errorStr);
    }
}

const TpmInfo::TpmData& TpmInfo::GetTpmData() const {
    return tpmData;
}

void TpmInfo::DetermineDetectionMethod() {
    // 根据检测结果确定使用的方法
    if (tpmData.tbsDetectionWorked && tpmData.wmiDetectionWorked) {
        tpmData.detectionMethod = L"TBS+WMI";
        Logger::Info("检测方法: TBS(主要) + WMI(备份详细信息)");
    } else if (tpmData.tbsDetectionWorked && !tpmData.wmiDetectionWorked) {
        tpmData.detectionMethod = L"TBS";
        Logger::Info("检测方法: TBS(仅)");
    } else if (!tpmData.tbsDetectionWorked && tpmData.wmiDetectionWorked) {
        tpmData.detectionMethod = L"WMI";
        Logger::Info("检测方法: WMI(备份方法)");
    } else {
        tpmData.detectionMethod = L"未检测到";
        Logger::Info("检测方法: 无 - 未检测到TPM");
        
        // 如果没有检测到TPM，设置适当的错误信息
        if (tpmData.errorMessage.empty()) {
            tpmData.errorMessage = L"TBS和WMI检测均失败";
        }
    }
}