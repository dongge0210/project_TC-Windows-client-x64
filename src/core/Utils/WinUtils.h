#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

class WinUtils {
public:
    // UTF-16 -> UTF-8（使用 -1，让 WideCharToMultiByte 自动包含终止符）
    static std::string WstringToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int size_needed = WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.c_str(), -1,      // -1: 包含终止符
            nullptr, 0,
            nullptr, nullptr
        );
        if (size_needed <= 0) return {};
        std::string out;
        out.resize(size_needed - 1); // 去掉 '\0'
        if (!out.empty()) {
            WideCharToMultiByte(
                CP_UTF8, 0,
                wstr.c_str(), -1,
                &out[0], size_needed,
                nullptr, nullptr
            );
        }
        return out;
    }

    // UTF-8 -> UTF-16
    static std::wstring Utf8ToWstring(const std::string& str) {
        if (str.empty()) return {};
        int size_needed = MultiByteToWideChar(
            CP_UTF8, 0,
            str.c_str(), -1,
            nullptr, 0
        );
        if (size_needed <= 0) return {};
        std::wstring out;
        out.resize(size_needed - 1);
        if (!out.empty()) {
            MultiByteToWideChar(
                CP_UTF8, 0,
                str.c_str(), -1,
                &out[0], size_needed
            );
        }
        return out;
    }

    // 兼容旧命名
    static std::wstring StringToWstring(const std::string& str) { return Utf8ToWstring(str); }
    static std::string  WstringToString(const std::wstring& wstr) { return WstringToUtf8(wstr); }

    // 简单 UTF-8 检测
    static bool IsLikelyUtf8(const std::string& s) {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
        size_t i = 0, len = s.size();
        while (i < len) {
            unsigned char c = bytes[i];
            if (c < 0x80) { ++i; continue; }
            size_t seqLen = 0;
            if      ((c & 0xE0) == 0xC0) seqLen = 2;
            else if ((c & 0xF0) == 0xE0) seqLen = 3;
            else if ((c & 0xF8) == 0xF0) seqLen = 4;
            else return false;
            if (i + seqLen > len) return false;
            for (size_t k = 1; k < seqLen; ++k)
                if ((bytes[i + k] & 0xC0) != 0x80) return false;
            i += seqLen;
        }
        return true;
    }

    static bool EnablePrivilege(const std::wstring& privilegeName, bool enable = true);
    static bool CheckPrivilege(const std::wstring& privilegeName);
    static bool IsRunAsAdmin();
    static std::string FormatWindowsErrorMessage(DWORD errorCode);
    static std::string GetExecutableDirectory();
};