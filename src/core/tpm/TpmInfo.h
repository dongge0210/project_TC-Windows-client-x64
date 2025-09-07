#pragma once
#include <vector>
#include <string>
#include <wbemidl.h>
#include <windows.h>

class WmiManager;

class TpmInfo {
public:
    struct TpmData {
        std::wstring manufacturerName;      // TPM 制造商
        std::wstring manufacturerId;        // 制造商ID
        std::wstring version;               // TPM版本
        std::wstring firmwareVersion;       // 固件版本
        bool isEnabled = false;             // TPM是否启用
        bool isActivated = false;           // TPM是否激活
        bool isOwned = false;               // TPM是否已拥有
        bool isReady = false;               // TPM是否就绪
        uint32_t specVersion = 0;           // TPM规范版本
        bool physicalPresenceRequired = false; // 是否需要物理存在
        std::wstring status;                // TPM状态
        
        // TBS (TPM Base Services) 信息
        bool tbsAvailable = false;          // TBS是否可用
        uint32_t tbsVersion = 0;            // TBS版本
        std::wstring errorMessage;          // 错误信息（如果有）
        
        // 检测方法信息
        std::wstring detectionMethod;       // 使用的检测方法 ("TBS", "WMI", "TBS+WMI", "未检测到")
        bool wmiDetectionWorked = false;    // WMI检测是否成功
        bool tbsDetectionWorked = false;    // TBS检测是否成功
    };

    TpmInfo(WmiManager& manager);
    ~TpmInfo();

    const TpmData& GetTpmData() const;
    bool HasTpm() const { return hasTpm; }

private:
    void DetectTpmViaWmi();
    void DetectTpmViaTbs();
    void QueryTpmProperties();
    void DetermineDetectionMethod(); // 确定使用的检测方法
    
    WmiManager& wmiManager;
    IWbemServices* pSvc = nullptr;
    TpmData tpmData;
    bool hasTpm = false;
};