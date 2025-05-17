#include "OSInfo.h"
#include "WinUtils.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <winternl.h> // 使用系统自带的RTL_OSVERSIONINFOW定义
#include <ntstatus.h>
#include <VersionHelpers.h>
#include <sstream>
#include <string>
#include <comdef.h>
#include <Wbemidl.h>
#include <comutil.h>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#pragma comment(lib, "wbemuuid.lib")

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

OSInfo::OSInfo() {
    Initialize();
}

void OSInfo::Initialize() {
    RTL_OSVERSIONINFOW osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    // 动态加载RtlGetVersion
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fn = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fn && fn(&osvi) == 0) {
            osVersion = WinUtils::WstringToString(
                std::wstring(L"Windows ") +
                std::to_wstring(osvi.dwMajorVersion) + L"." +
                std::to_wstring(osvi.dwMinorVersion) +
                L" (Build " + std::to_wstring(osvi.dwBuildNumber) + L")"
            );
        } else {
            osVersion = "Unknown OS Version";
        }
    } else {
        osVersion = "Unknown OS Version";
    }
    osDetailedVersion = GetDetailedVersion();
}

std::string OSInfo::GetVersion() const {
    return osVersion;
}

std::string OSInfo::GetDetailedVersion() const {
    RTL_OSVERSIONINFOW rovi = { 0 };
    rovi.dwOSVersionInfoSize = sizeof(rovi);
    std::string result;
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fn = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fn && fn(&rovi) == 0) {
            std::wstringstream ws;
            ws << L"Windows " << rovi.dwMajorVersion << L"." << rovi.dwMinorVersion
               << L" (Build " << rovi.dwBuildNumber << L")";
            if (rovi.szCSDVersion[0]) {
                ws << L" " << rovi.szCSDVersion;
            }
            // Use WinUtils::WstringToString for conversion
            result = WinUtils::WstringToString(ws.str());
        } else {
            result = "Unknown OS Version";
        }
    } else {
        result = "Unknown OS Version";
    }
    return result;
}

std::string OSInfo::GetMotherboardName() const {
    // WMI查询主板
    HRESULT hres;
    std::string result = "未知";
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return result;

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);
    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return result;
    }

    hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return result;
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return result;
    }

    hres = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return result;
    }

    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT Product, Manufacturer FROM Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (SUCCEEDED(hres) && pEnumerator) {
        IWbemClassObject* pclsObj = nullptr;
        ULONG uReturn = 0;
        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            VARIANT vtProp1, vtProp2;
            VariantInit(&vtProp1); VariantInit(&vtProp2);
            pclsObj->Get(L"Manufacturer", 0, &vtProp1, 0, 0);
            pclsObj->Get(L"Product", 0, &vtProp2, 0, 0);
            std::string manufacturer = (vtProp1.vt == VT_BSTR) ? (const char*)_bstr_t(vtProp1.bstrVal) : "";
            std::string product = (vtProp2.vt == VT_BSTR) ? (const char*)_bstr_t(vtProp2.bstrVal) : "";
            result = manufacturer + " " + product;
            VariantClear(&vtProp1); VariantClear(&vtProp2);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return result;
}

std::string OSInfo::GetDeviceName() const {
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return std::string(buffer);
    }
    return "未知";
}