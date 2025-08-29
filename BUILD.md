# Cross-Platform Build Guide

This document explains how to build the SystemMonitor application on different platforms and how to resolve common compilation issues and library dependencies.

## Platform Support

- **Windows**: Full feature support with WMI monitoring, shared memory, temperature sensors, and all complex monitoring features
- **Linux**: Cross-platform system monitoring with basic CPU, memory, and OS information
- **macOS**: Cross-platform system monitoring with macOS-specific system calls

## Prerequisites

### Common Requirements
- CMake 3.16 or higher
- C++17 compatible compiler
- Git (for cloning the repository)

### Platform-Specific Requirements

#### Windows
- **Compiler**: Visual Studio 2019/2022 or MinGW-w64
- **System Libraries** (automatically linked):
  - `kernel32.lib` - Core Windows API
  - `user32.lib` - User interface functions
  - `advapi32.lib` - Advanced Windows API (registry, services)
  - `shell32.lib` - Shell functions
  - `ole32.lib` - COM/OLE support
  - `oleaut32.lib` - OLE Automation
  - `uuid.lib` - GUID support
  - `wbemuuid.lib` - WMI support
  - `pdh.lib` - Performance Data Helper
  - `iphlpapi.lib` - IP Helper API
  - `ws2_32.lib` - Winsock 2.0

#### Linux
- **Compiler**: GCC 7+ or Clang 6+
- **System Libraries**:
  - `pthread` (POSIX threads) - **Required**
  - `librt` (Real-time extensions) - *Optional, for better timing*
- **Package Manager Commands**:
  ```bash
  # Ubuntu/Debian
  sudo apt update
  sudo apt install build-essential cmake libpthread-stubs0-dev
  
  # CentOS/RHEL/Fedora
  sudo yum groupinstall "Development Tools"
  sudo yum install cmake
  # or for newer versions:
  sudo dnf groupinstall "Development Tools"
  sudo dnf install cmake
  
  # Arch Linux
  sudo pacman -S base-devel cmake
  ```

#### macOS
- **Compiler**: Xcode Command Line Tools or Xcode IDE
- **System Frameworks** (automatically found):
  - `CoreFoundation` - Core system services
  - `IOKit` - Hardware access and device drivers
- **Setup Commands**:
  ```bash
  # Install Xcode Command Line Tools
  xcode-select --install
  
  # Install CMake (using Homebrew)
  brew install cmake
  ```

## Building Instructions

### 1. Clone and Prepare
```bash
git clone <repository-url>
cd project_TC-Windows-client-x64
```

### 2. Build Commands

#### All Platforms (Standard CMake approach)
```bash
# Create build directory
mkdir build && cd build

# Configure the build
cmake ..

# Build the project
cmake --build . --config Release

# Alternative: use make on Unix systems
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
```

#### Windows-Specific (Visual Studio)
```bash
# Generate Visual Studio project files
cmake .. -G "Visual Studio 16 2019" -A x64

# Build using MSBuild
msbuild SystemMonitor.sln /p:Configuration=Release
```

#### Linux/macOS with specific compiler
```bash
# Using GCC
cmake .. -DCMAKE_CXX_COMPILER=g++

# Using Clang
cmake .. -DCMAKE_CXX_COMPILER=clang++
```

## Troubleshooting Common Issues

### 1. Missing System Libraries

#### Problem: "undefined reference to pthread_create" (Linux)
**Solution**:
```bash
sudo apt install libpthread-stubs0-dev  # Ubuntu/Debian
sudo yum install glibc-devel            # CentOS/RHEL
```

#### Problem: "CoreFoundation framework not found" (macOS)
**Solution**:
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Verify installation
xcode-select -p
```

#### Problem: Windows libraries not found
**Solution**:
- Ensure you're using Visual Studio Developer Command Prompt
- Install Windows SDK (usually comes with Visual Studio)
- For MinGW, ensure you have the complete mingw-w64 toolchain

### 2. Compiler Version Issues

#### Problem: C++17 not supported
**Solution**:
```bash
# Check compiler version
g++ --version      # Should be 7.0+
clang++ --version  # Should be 6.0+

# Ubuntu: upgrade compiler
sudo apt install gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
```

### 3. CMake Version Issues

#### Problem: CMake version too old
**Solution**:
```bash
# Ubuntu: install newer CMake
sudo apt remove cmake
sudo apt install cmake-data
wget https://github.com/Kitware/CMake/releases/download/v3.22.0/cmake-3.22.0-linux-x86_64.sh
sudo sh cmake-3.22.0-linux-x86_64.sh --prefix=/usr/local --skip-license

# macOS: upgrade via Homebrew
brew upgrade cmake

# Windows: download from cmake.org
```

### 4. Platform Detection Issues

If CMake doesn't detect your platform correctly, you can force it:

```bash
# Force Windows build
cmake .. -DWIN32=ON

# Force macOS build
cmake .. -DAPPLE=ON

# Force Linux build
cmake .. -DUNIX=ON -DAPPLE=OFF
```

## Build Configuration Options

### Debug vs Release
```bash
# Debug build (with debugging symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Verbose Build Output
```bash
# See detailed compilation commands
cmake --build . --verbose

# Or with make
make VERBOSE=1
```

### Static vs Dynamic Linking
```bash
# Static linking (reduces dependencies)
cmake .. -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
```

## Testing the Build

### Basic Functionality Test
```bash
# Run the built executable
./SystemMonitor          # Linux/macOS
SystemMonitor.exe        # Windows

# Expected output should show:
# - Platform detection
# - OS version information  
# - CPU information
# - Memory usage
# - Real-time monitoring data
```

### Platform-Specific Testing

#### Windows
- Should show WMI data
- Shared memory functionality
- Temperature sensors (if available)
- All advanced monitoring features

#### Linux/macOS  
- Should show cross-platform system info
- CPU usage monitoring
- Memory statistics
- Basic system monitoring

## Advanced Build Options

### Cross-Compilation

#### Linux to Windows (using MinGW)
```bash
sudo apt install mingw-w64
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake
```

#### Custom Install Location
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make install
```

## Performance Optimizations

### Compiler Optimizations
```bash
# Maximum optimization
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Size optimization
cmake .. -DCMAKE_CXX_FLAGS="-Os -DNDEBUG"
```

### Memory Usage
```bash
# Reduce memory usage during compilation
make -j1  # Use single thread instead of -j$(nproc)
```

## Support and Issues

If you encounter build issues not covered here:

1. **Check Dependencies**: Ensure all required libraries are installed
2. **Update Tools**: Use recent versions of CMake, compiler, and system libraries
3. **Clean Build**: Remove `build/` directory and rebuild from scratch
4. **Platform Support**: Remember that some features are Windows-only
5. **Compiler Warnings**: Current build produces only warnings, not errors

The cross-platform implementation maintains full Windows functionality while providing equivalent monitoring capabilities on Linux and macOS using platform-native APIs.