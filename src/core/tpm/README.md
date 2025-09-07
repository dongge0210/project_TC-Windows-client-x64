# TPM Detection Module

## Overview
This module provides TPM (Trusted Platform Module) detection and information gathering for Windows systems using both WMI and TBS (TPM Base Services) APIs.

## Features

### Detection Methods
1. **WMI Detection**: Uses `Win32_Tpm` WMI class for basic TPM information
2. **TBS Detection**: Uses Windows TPM Base Services for detailed TPM capabilities

### Information Collected
- TPM manufacturer name and ID
- TPM specification version (1.2 or 2.0)
- TPM status (enabled/activated/ready)
- TBS service availability
- Physical presence requirements
- Error conditions and diagnostics

## Usage

### In Application Code
```cpp
#include "core/tpm/TpmInfo.h"

// Initialize with WMI manager
WmiManager wmiManager;
TpmInfo tpmInfo(wmiManager);

// Check if TPM is available
if (tpmInfo.HasTpm()) {
    const auto& tpmData = tpmInfo.GetTpmData();
    
    std::wcout << L"TPM Manufacturer: " << tpmData.manufacturerName << std::endl;
    std::wcout << L"TPM Version: " << tpmData.version << std::endl;
    std::wcout << L"TPM Status: " << tpmData.status << std::endl;
    std::cout << "TPM Enabled: " << (tpmData.isEnabled ? "Yes" : "No") << std::endl;
    std::cout << "TPM Ready: " << (tpmData.isReady ? "Yes" : "No") << std::endl;
}
```

### In Shared Memory
TPM information is automatically included in the shared memory block and can be accessed by other processes:

```cpp
SharedMemoryBlock* buffer = SharedMemoryManager::GetBuffer();
if (buffer && buffer->hasTpm) {
    // Access TPM data from shared memory
    const TmpData& tmpData = buffer->tmp;
    // Use tmpData.manufacturerName, tmpData.version, etc.
}
```

## Data Structure

### TmpData Structure
```cpp
struct TmpData {
    wchar_t manufacturerName[128];  // TPM manufacturer name
    wchar_t manufacturerId[32];     // Manufacturer ID  
    wchar_t version[32];            // TPM version
    wchar_t firmwareVersion[32];    // Firmware version
    wchar_t status[64];             // TPM status
    bool isEnabled;                 // TPM enabled
    bool isActivated;               // TPM activated
    bool isOwned;                   // TPM owned
    bool isReady;                   // TPM ready
    bool tbsAvailable;              // TBS available
    bool physicalPresenceRequired;  // Physical presence required
    uint32_t specVersion;           // TPM spec version
    uint32_t tbsVersion;            // TBS version
    wchar_t errorMessage[256];      // Error message
};
```

## Error Handling
The module handles various error conditions:
- **No TPM Hardware**: When no TPM is present or detected
- **TPM Disabled**: When TPM is present but disabled in BIOS/UEFI
- **Service Issues**: When TPM service is not running
- **Access Denied**: When insufficient privileges to access TPM

## Dependencies
- Windows SDK (for TBS APIs)
- WMI support
- `tbs.lib` (automatically linked)

## Limitations
- Windows-only (uses Windows-specific TBS APIs)
- Requires administrator privileges for full TPM information
- Some information may not be available depending on TPM configuration

## Integration
The TPM detection is integrated into the main system monitoring loop and runs once during initialization. The results are cached and included in system information reports.