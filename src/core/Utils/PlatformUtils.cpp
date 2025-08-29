#include "PlatformUtils.h"

#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <shellapi.h>
    #include <sddl.h>
    #include <Aclapi.h>
#elif PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <cstring>
    #include <errno.h>
    #include <limits.h>
    #include <pwd.h>
#elif PLATFORM_MACOS
    #include <unistd.h>
    #include <sys/types.h>
    #include <cstring>
    #include <errno.h>
    #include <mach-o/dyld.h>
    #include <pwd.h>
#endif

#include <algorithm>
#include <codecvt>
#include <locale>

// String conversion implementations
std::string PlatformUtils::WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    
#if PLATFORM_WINDOWS
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return {};
    std::string out(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), out.data(), size_needed, nullptr, nullptr);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
#endif
}

std::wstring PlatformUtils::Utf8ToWstring(const std::string& str) {
    if (str.empty()) return {};
    
#if PLATFORM_WINDOWS
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    if (size_needed <= 0) return {};
    std::wstring out(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), out.data(), size_needed);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
#endif
}

// Privilege and admin check implementations
bool PlatformUtils::EnablePrivilege(const std::wstring& privilegeName, bool enable) {
#if PLATFORM_WINDOWS
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return false;
    }
    LUID luid;
    if (!LookupPrivilegeValueW(nullptr, privilegeName.c_str(), &luid)) {
        CloseHandle(token);
        return false;
    }
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        CloseHandle(token);
        return false;
    }
    CloseHandle(token);
    return (GetLastError() == ERROR_SUCCESS);
#else
    // On Unix systems, privilege management is different
    // This is mainly a Windows concept, so return false on other platforms
    (void)privilegeName; // Suppress unused parameter warning
    (void)enable;        // Suppress unused parameter warning
    return false;
#endif
}

bool PlatformUtils::CheckPrivilege(const std::wstring& privilegeName) {
#if PLATFORM_WINDOWS
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }

    LUID luid;
    if (!LookupPrivilegeValue(NULL, privilegeName.c_str(), &luid)) {
        CloseHandle(hToken);
        return false;
    }

    PRIVILEGE_SET ps;
    ps.PrivilegeCount = 1;
    ps.Control = PRIVILEGE_SET_ALL_NECESSARY;
    ps.Privilege[0].Luid = luid;
    ps.Privilege[0].Attributes = 0;

    BOOL result = FALSE;
    BOOL checkResult = PrivilegeCheck(hToken, &ps, &result);
    
    CloseHandle(hToken);
    return (checkResult && result);
#else
    (void)privilegeName; // Suppress unused parameter warning
    return false;
#endif
}

bool PlatformUtils::IsRunAsAdmin() {
#if PLATFORM_WINDOWS
    BOOL isAdmin = FALSE;
    PSID adminGroupSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroupSid)) {
        return false;
    }
    
    if (!CheckTokenMembership(NULL, adminGroupSid, &isAdmin)) {
        isAdmin = FALSE;
    }
    
    FreeSid(adminGroupSid);
    return (isAdmin != FALSE);
#else
    return (geteuid() == 0);
#endif
}

// Error message formatting
std::string PlatformUtils::FormatErrorMessage(int errorCode) {
#if PLATFORM_WINDOWS
    return FormatWindowsErrorMessage((DWORD)errorCode);
#elif PLATFORM_LINUX
    return FormatLinuxErrorMessage(errorCode);
#elif PLATFORM_MACOS
    return FormatMacErrorMessage(errorCode);
#endif
}

#if PLATFORM_WINDOWS
std::string PlatformUtils::FormatWindowsErrorMessage(DWORD errorCode) {
    LPWSTR buffer = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );
    if (!buffer) {
        return "Unknown error: " + std::to_string(errorCode);
    }
    std::wstring wideMsg(buffer);
    LocalFree(buffer);
    return WstringToUtf8(wideMsg);
}
#elif PLATFORM_LINUX
std::string PlatformUtils::FormatLinuxErrorMessage(int errorCode) {
    return std::string(strerror(errorCode)) + " (" + std::to_string(errorCode) + ")";
}
#elif PLATFORM_MACOS
std::string PlatformUtils::FormatMacErrorMessage(int errorCode) {
    return std::string(strerror(errorCode)) + " (" + std::to_string(errorCode) + ")";
}
#endif

// Directory utilities
std::string PlatformUtils::GetExecutableDirectory() {
#if PLATFORM_WINDOWS
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos);
    }
    return fullPath;
#elif PLATFORM_LINUX
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("/");
        if (pos != std::string::npos) {
            return fullPath.substr(0, pos);
        }
    }
    return ".";
#elif PLATFORM_MACOS
    char path[PATH_MAX];
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("/");
        if (pos != std::string::npos) {
            return fullPath.substr(0, pos);
        }
    }
    return ".";
#endif
}

// Platform identification
std::string PlatformUtils::GetPlatformName() {
#if PLATFORM_WINDOWS
    return "Windows";
#elif PLATFORM_LINUX
    return "Linux";
#elif PLATFORM_MACOS
    return "macOS";
#endif
}

// UTF-8 validation (same logic as WinUtils)
bool PlatformUtils::IsLikelyUtf8(const std::string& s) {
    // Simple UTF-8 validation - check for valid UTF-8 sequences
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();
    
    for (size_t i = 0; i < len; ++i) {
        if (bytes[i] < 0x80) {
            continue; // ASCII
        } else if ((bytes[i] >> 5) == 0x06) {
            // 110xxxxx - 2 byte sequence
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
        } else if ((bytes[i] >> 4) == 0x0E) {
            // 1110xxxx - 3 byte sequence  
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
        } else if ((bytes[i] >> 3) == 0x1E) {
            // 11110xxx - 4 byte sequence
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
            if (++i >= len || (bytes[i] >> 6) != 0x02) return false;
        } else {
            return false;
        }
    }
    return true;
}