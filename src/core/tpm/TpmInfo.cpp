#include "TpmInfo.h"
#include "../utils/Logger.h"
#include "../utils/WmiManager.h"
#include <comutil.h>
#include <tbs.h>        // TPM Base Services
#include <wbemidl.h>

// 链接TBS库
#pragma comment(lib, "tbs.lib")

TpmInfo::TpmInfo(WmiManager& manager) : wmiManager(manager) {
    if (!wmiManager.IsInitialized()) {
        Logger::Error("WMI服务未初始化");
        return;
    }
    pSvc = wmiManager.GetWmiService();
    
    // 首先尝试通过WMI检测TPM
    DetectTpmViaWmi();
    
    // 然后尝试通过TBS检测TPM
    DetectTpmViaTbs();
    
    Logger::Info("TPM检测完成");
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
    
    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
        hasTpm = true;
        
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
            tpmData.status = L"就绪";
        } else if (tpmData.isEnabled && !tpmData.isActivated) {
            tpmData.status = L"已启用但未激活";
        } else if (!tpmData.isEnabled) {
            tpmData.status = L"未启用";
        } else {
            tpmData.status = L"未知状态";
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
        
        Logger::Info("检测到TPM: " + manufacturerStr + 
                    ", 版本: " + versionStr + 
                    ", 状态: " + statusStr + 
                    ", 已启用: " + (tpmData.isEnabled ? "是" : "否") +
                    ", 已激活: " + (tpmData.isActivated ? "是" : "否"));
        
        break; // 只处理第一个TPM
    }

    pEnumerator->Release();

    if (!hasTpm) {
        Logger::Info("通过WMI未检测到TPM设备");
    }
}

void TpmInfo::DetectTpmViaTbs() {
    // 尝试检测TBS (TPM Base Services)
    TBS_HCONTEXT hContext = 0;
    TBS_CONTEXT_PARAMS2 contextParams = {0};
    contextParams.version = TBS_CONTEXT_VERSION_TWO;
    contextParams.requestRaw = FALSE;

    TBS_RESULT result = Tbsi_Context_Create(&contextParams, &hContext);
    
    if (result == TBS_SUCCESS) {
        tpmData.tbsAvailable = true;
        
        // 获取TBS版本信息
        TPM_DEVICE_INFO deviceInfo = {0};
        UINT32 deviceInfoSize = sizeof(deviceInfo);
        
        result = Tbsi_GetDeviceInfo(sizeof(deviceInfo), &deviceInfo);
        if (result == TBS_SUCCESS) {
            tpmData.tbsVersion = deviceInfo.tpmVersion;
            
            // 根据TPM版本设置版本字符串
            if (deviceInfo.tpmVersion == TPM_VERSION_12) {
                if (tpmData.version.empty()) {
                    tpmData.version = L"1.2";
                }
            } else if (deviceInfo.tpmVersion == TPM_VERSION_20) {
                if (tpmData.version.empty()) {
                    tpmData.version = L"2.0";
                }
            }
            
            Logger::Info("TBS可用, TPM版本: " + std::to_string(deviceInfo.tpmVersion));
        }
        
        // 如果WMI没有检测到TPM，但TBS可用，说明TPM存在
        if (!hasTpm) {
            hasTpm = true;
            tpmData.isEnabled = true; // TBS可用说明TPM已启用
            tpmData.status = L"通过TBS检测到";
            Logger::Info("通过TBS检测到TPM");
        }
        
        Tbsi_Context_Close(hContext);
    } else {
        tpmData.tbsAvailable = false;
        
        // 设置错误信息
        switch (result) {
            case TBS_E_TPM_NOT_FOUND:
                tpmData.errorMessage = L"未找到TPM设备";
                break;
            case TBS_E_SERVICE_NOT_RUNNING:
                tpmData.errorMessage = L"TPM服务未运行";
                break;
            case TBS_E_INSUFFICIENT_BUFFER:
                tpmData.errorMessage = L"缓冲区不足";
                break;
            default:
                tpmData.errorMessage = L"TBS初始化失败: 0x" + std::to_wstring(result);
                break;
        }
        
        std::string errorStr(tpmData.errorMessage.begin(), tpmData.errorMessage.end());
        Logger::Warn("TBS检测失败: " + errorStr);
    }
}

const TpmInfo::TpmData& TpmInfo::GetTpmData() const {
    return tpmData;
}