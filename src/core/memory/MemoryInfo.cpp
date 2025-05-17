#include "MemoryInfo.h"
#include "../temperature/LibreHardwareMonitorBridge.h"
#include <windows.h>
#include <wbemidl.h>
#include <codecvt>

#pragma comment(lib, "wbemuuid.lib")

MemoryInfo::MemoryInfo() {
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    // 改为调用自身的方法检测内存频率
    memoryFrequency = DetectMemoryFrequencyMHz();
}

uint32_t MemoryInfo::DetectMemoryFrequencyMHz() {
    // 使用静态变量缓存结果，避免重复查询
    static uint32_t cachedFreq = 0;
    static bool hasQueried = false;

    // 如果已经查询过，直接返回缓存结果
    if (hasQueried) {
        return cachedFreq;
    }

    // 标记为已查询
    hasQueried = true;

    // 尝试通过WMI获取内存频率
    uint32_t maxFreq = GetMemoryFrequencyViaWMI();

    // 如果WMI获取失败，使用CPU型号估算
    if (maxFreq == 0) {
        maxFreq = EstimateMemoryFrequencyFromCPU();
    }

    // 缓存结果
    cachedFreq = maxFreq;
    return maxFreq;
}

uint32_t MemoryInfo::GetMemoryFrequencyViaWMI() {
    HRESULT hr;
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    uint32_t maxFreq = 0;

    try {
        // 初始化COM
        hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
            CoUninitialize();
            return 0;
        }

        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hr) || !pLoc) {
            CoUninitialize();
            return 0;
        }

        hr = pLoc->ConnectServer(
            BSTR(L"ROOT\\CIMV2"),
            nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hr) || !pSvc) {
            if (pLoc) pLoc->Release();
            CoUninitialize();
            return 0;
        }

        hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

        if (FAILED(hr)) {
            if (pSvc) pSvc->Release();
            if (pLoc) pLoc->Release();
            CoUninitialize();
            return 0;
        }

        hr = pSvc->ExecQuery(
            BSTR(L"WQL"),
            BSTR(L"SELECT Speed, ConfiguredClockSpeed FROM Win32_PhysicalMemory"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &pEnumerator);

        if (SUCCEEDED(hr) && pEnumerator) {
            IWbemClassObject* pObj = nullptr;
            ULONG uReturn = 0;
            uint32_t wmiMaxFreq = 0;

            while (pEnumerator->Next(WBEM_INFINITE, 1, &pObj, &uReturn) == S_OK) {
                // 尝试Speed属性
                VARIANT vtSpeed;
                VariantInit(&vtSpeed);
                if (SUCCEEDED(pObj->Get(L"Speed", 0, &vtSpeed, 0, 0)) && vtSpeed.vt == VT_I4) {
                    uint32_t freq = static_cast<uint32_t>(vtSpeed.lVal);
                    if (freq > 0 && freq < 10000 && freq > wmiMaxFreq) {
                        wmiMaxFreq = freq;
                    }
                }
                VariantClear(&vtSpeed);

                // 尝试ConfiguredClockSpeed属性
                VARIANT vtConfigSpeed;
                VariantInit(&vtConfigSpeed);
                if (SUCCEEDED(pObj->Get(L"ConfiguredClockSpeed", 0, &vtConfigSpeed, 0, 0)) && vtConfigSpeed.vt == VT_I4) {
                    uint32_t freq = static_cast<uint32_t>(vtConfigSpeed.lVal);
                    if (freq > 0 && freq < 10000 && freq > wmiMaxFreq) {
                        wmiMaxFreq = freq;
                    }
                }
                VariantClear(&vtConfigSpeed);

                pObj->Release();
            }

            if (wmiMaxFreq > 0) {
                maxFreq = wmiMaxFreq;
            }

            pEnumerator->Release();
        }

        if (pSvc) pSvc->Release();
        if (pLoc) pLoc->Release();
        CoUninitialize();
    }
    catch (const std::exception&) {
        if (pEnumerator) pEnumerator->Release();
        if (pSvc) pSvc->Release();
        if (pLoc) pLoc->Release();
        CoUninitialize();
    }

    return maxFreq;
}

// 根据CPU型号估计内存频率
uint32_t MemoryInfo::EstimateMemoryFrequencyFromCPU() {
    // 本地缓存CPU名称
    std::string cpuName;

    // 从注册表获取 CPU 信息
    HKEY hKey;
    DWORD dwType = REG_SZ;
    DWORD dwSize = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueEx(hKey, L"ProcessorNameString", NULL, &dwType, NULL, &dwSize);
        if (dwSize > 0) {
            std::wstring buffer(dwSize / sizeof(wchar_t), 0);
            if (RegQueryValueEx(hKey, L"ProcessorNameString", NULL, &dwType, (LPBYTE)&buffer[0], &dwSize) == ERROR_SUCCESS) {
                // 转换为std::string
                std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
                cpuName = conv.to_bytes(buffer.c_str());
            }
        }
        RegCloseKey(hKey);
    }

    // 检查CPU代数以粗略估计可能的内存频率
    if (cpuName.find("13th Gen") != std::string::npos ||  // 13代酷睿
        cpuName.find("14th Gen") != std::string::npos ||  // 14代酷睿
        cpuName.find("Ryzen 7000") != std::string::npos || // Ryzen 7000系列
        cpuName.find("Ryzen 8000") != std::string::npos) { // Ryzen 8000系列

        // 新平台很可能使用DDR5
        return 5600; // 常见DDR5-5600
    }
    else if (cpuName.find("12th Gen") != std::string::npos || // 12代酷睿
        cpuName.find("11th Gen") != std::string::npos || // 11代酷睿
        cpuName.find("Ryzen 5000") != std::string::npos || // Ryzen 5000系列
        cpuName.find("Ryzen 6000") != std::string::npos) { // Ryzen 6000系列

        // 较新平台可能使用DDR4-3200或DDR4-3600
        return 3600; // 较高频率DDR4-3600
    }
    else if (cpuName.find("10th Gen") != std::string::npos || // 10代酷睿
        cpuName.find("9th Gen") != std::string::npos ||  // 9代酷睿
        cpuName.find("Ryzen 3000") != std::string::npos) { // Ryzen 3000系列

        // 稍旧平台可能使用DDR4-3000或以下
        return 3000; // 标准DDR4-3000
    }
    else {
        // 无法确定，给一个保守估计
        return 2666; // 保守的DDR4-2666
    }
}

ULONGLONG MemoryInfo::GetTotalPhysical() const {
    return memStatus.ullTotalPhys;
}

ULONGLONG MemoryInfo::GetAvailablePhysical() const {
    return memStatus.ullAvailPhys;
}

ULONGLONG MemoryInfo::GetTotalVirtual() const {
    return memStatus.ullTotalVirtual;
}

uint32_t MemoryInfo::GetMemoryFrequency() const {
    return memoryFrequency;
}
