#pragma once
#include <string>
#include <windows.h>

class OSInfo {
public:
    OSInfo();
    std::string GetVersion() const;
    std::string GetDetailedVersion() const;
    std::string GetMotherboardName() const;
    std::string GetDeviceName() const;
    void Initialize();
private:
    std::string osVersion;
    std::string osDetailedVersion;
    DWORD majorVersion;
    DWORD minorVersion;
    DWORD buildNumber;
};
