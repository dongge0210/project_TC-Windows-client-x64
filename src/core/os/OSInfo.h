#pragma once
#include <string>
#include <windows.h>

class OSInfo {
public:
    OSInfo();
    std::string GetVersion() const;
    std::string GetDetailedVersion() const; // 新增：详细系统版本
    void Initialize();
private:
    std::string osVersion;
    std::string osDetailedVersion; // 新增
    DWORD majorVersion;
    DWORD minorVersion;
    DWORD buildNumber;
};
