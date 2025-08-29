#pragma once
#include <string>
#include "../platform/Platform.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#endif

class OSInfo {
public:
    OSInfo();
    std::string GetVersion() const;
    void Initialize();
private:
    std::string osVersion;
#if PLATFORM_WINDOWS
    DWORD majorVersion;
    DWORD minorVersion;
    DWORD buildNumber;
#else
    int majorVersion;
    int minorVersion;
    int buildNumber;
#endif
};
