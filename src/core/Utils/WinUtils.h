#pragma once
#include <windows.h>
#include <string>

#ifdef QT_CORE_LIB
#include <QString>
#include <QDebug> // 显式包含QDebug，防止QDebug未定义
#endif

// 保证没有与comutil.h冲突的类型、宏、using等
// 不要定义Data_t、operator=、operator+等与comutil.h同名的内容

// Utility class for Windows-specific operations
class WinUtils {
public:
    // Convert std::string to std::wstring
    static std::wstring StringToWstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), 
            static_cast<int>(str.size()), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
            &wstr[0], size_needed);
        return wstr;
    }
    
    // Convert std::wstring to std::string
    static std::string WstringToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
            static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
            &str[0], size_needed, NULL, NULL);
        return str;
    }
    
    // Enable a privilege by name (such as "SeCreateGlobalPrivilege")
    static bool EnablePrivilege(const std::wstring& privilegeName, bool enable = true);
    
    // Overload for wchar_t* version
    static bool EnablePrivilege(const wchar_t* privilegeName);
    
    // Check if the current process has a specific privilege enabled
    static bool CheckPrivilege(const std::wstring& privilegeName);
    
    // Check if the current process is running with administrator privileges
    static bool IsRunAsAdmin();
    
    // Format Windows error code to message string
    static std::string FormatWindowsErrorMessage(DWORD errorCode);

    // 添加在其他转换函数附近
    static std::string WstringToUtf8String(const std::wstring& wstr);

    // Check if the current process is elevated
    static bool IsProcessElevated();

    // Check if the current user is an administrator
    static bool IsUserAdmin();

#ifdef QT_CORE_LIB
    // 建议：提供与 Qt 交互的安全转换函数
    static QString WstringToQString(const std::wstring& wstr);
    static QString Utf8StringToQString(const std::string& str);
#endif
};
