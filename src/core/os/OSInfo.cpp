#include "OSInfo.h"
#include "WinUtils.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <winternl.h> // 使用系统自带的RTL_OSVERSIONINFOW定义
#include <ntstatus.h>
#include <VersionHelpers.h>
#include <sstream>
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

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