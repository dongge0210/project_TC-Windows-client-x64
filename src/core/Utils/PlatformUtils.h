#pragma once

#include "../platform/Platform.h"

class PlatformUtils {
public:
    // String conversion utilities (UTF-8 based for cross-platform compatibility)
    static std::string WstringToUtf8(const std::wstring& wstr);
    static std::wstring Utf8ToWstring(const std::string& str);
    
    // Compatibility aliases
    static std::wstring StringToWstring(const std::string& str) { return Utf8ToWstring(str); }
    static std::string WstringToString(const std::wstring& wstr) { return WstringToUtf8(wstr); }

    // Platform-specific privilege and admin checks
    static bool EnablePrivilege(const std::wstring& privilegeName, bool enable = true);
    static bool CheckPrivilege(const std::wstring& privilegeName);
    static bool IsRunAsAdmin();
    
    // Error message formatting
    static std::string FormatErrorMessage(int errorCode);
    
    // Directory utilities
    static std::string GetExecutableDirectory();
    
    // Platform identification
    static std::string GetPlatformName();
    
    // UTF-8 validation
    static bool IsLikelyUtf8(const std::string& s);

private:
#if PLATFORM_WINDOWS
    static std::string FormatWindowsErrorMessage(DWORD errorCode);
#elif PLATFORM_LINUX
    static std::string FormatLinuxErrorMessage(int errorCode);
#elif PLATFORM_MACOS  
    static std::string FormatMacErrorMessage(int errorCode);
#endif
};