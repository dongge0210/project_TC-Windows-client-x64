#!/bin/bash

# Cross-Platform Build Troubleshooting Script
# This script helps diagnose and resolve compilation issues for the SystemMonitor project

set -e

echo "SystemMonitor Build Troubleshooting Script"
echo "=========================================="
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

# Detect platform
detect_platform() {
    echo "1. Platform Detection"
    echo "-------------------"
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="Linux"
        PACKAGE_MANAGER="apt"
        if command -v yum >/dev/null 2>&1; then
            PACKAGE_MANAGER="yum"
        elif command -v dnf >/dev/null 2>&1; then
            PACKAGE_MANAGER="dnf"
        elif command -v pacman >/dev/null 2>&1; then
            PACKAGE_MANAGER="pacman"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macOS"
        PACKAGE_MANAGER="brew"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        PLATFORM="Windows"
        PACKAGE_MANAGER="chocolatey"
    else
        PLATFORM="Unknown"
        PACKAGE_MANAGER="unknown"
    fi
    
    print_status "Detected platform: $PLATFORM"
    print_info "Package manager: $PACKAGE_MANAGER"
    echo
}

# Check system requirements
check_requirements() {
    echo "2. System Requirements Check"
    echo "---------------------------"
    
    # Check CMake
    if command -v cmake >/dev/null 2>&1; then
        CMAKE_VERSION=$(cmake --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
        CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
        CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
        
        if [[ $CMAKE_MAJOR -gt 3 ]] || [[ $CMAKE_MAJOR -eq 3 && $CMAKE_MINOR -ge 16 ]]; then
            print_status "CMake $CMAKE_VERSION (✓ >= 3.16)"
        else
            print_error "CMake $CMAKE_VERSION (✗ < 3.16 required)"
            suggest_cmake_upgrade
        fi
    else
        print_error "CMake not found"
        suggest_cmake_install
    fi
    
    # Check C++ compiler
    if command -v g++ >/dev/null 2>&1; then
        GCC_VERSION=$(g++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
        print_status "GCC $GCC_VERSION found"
    elif command -v clang++ >/dev/null 2>&1; then
        CLANG_VERSION=$(clang++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -1)
        print_status "Clang $CLANG_VERSION found"
    else
        print_error "No C++ compiler found"
        suggest_compiler_install
    fi
    
    # Check Make
    if command -v make >/dev/null 2>&1; then
        print_status "Make utility found"
    else
        print_warning "Make utility not found"
    fi
    
    echo
}

# Check platform-specific dependencies
check_dependencies() {
    echo "3. Platform-Specific Dependencies"
    echo "---------------------------------"
    
    case $PLATFORM in
        "Linux")
            check_linux_deps
            ;;
        "macOS")
            check_macos_deps
            ;;
        "Windows")
            check_windows_deps
            ;;
        *)
            print_warning "Unknown platform - manual dependency check required"
            ;;
    esac
    echo
}

check_linux_deps() {
    # Check pthread
    if ldconfig -p | grep -q libpthread; then
        print_status "pthread library found"
    else
        print_error "pthread library not found"
        echo "  Install with: sudo $PACKAGE_MANAGER install libpthread-stubs0-dev"
    fi
    
    # Check librt (optional)
    if ldconfig -p | grep -q librt; then
        print_status "librt library found (optional)"
    else
        print_warning "librt library not found (optional - provides better timing)"
    fi
    
    # Check build tools
    if command -v gcc >/dev/null 2>&1; then
        print_status "Build tools installed"
    else
        print_error "Build tools not installed"
        case $PACKAGE_MANAGER in
            "apt")
                echo "  Install with: sudo apt install build-essential"
                ;;
            "yum"|"dnf")
                echo "  Install with: sudo $PACKAGE_MANAGER groupinstall \"Development Tools\""
                ;;
            "pacman")
                echo "  Install with: sudo pacman -S base-devel"
                ;;
        esac
    fi
}

check_macos_deps() {
    # Check Xcode Command Line Tools
    if xcode-select -p >/dev/null 2>&1; then
        print_status "Xcode Command Line Tools installed"
    else
        print_error "Xcode Command Line Tools not installed"
        echo "  Install with: xcode-select --install"
    fi
    
    # Check frameworks (these should be available on all macOS systems)
    print_status "CoreFoundation and IOKit frameworks (built-in)"
    
    # Check Homebrew (recommended but not required)
    if command -v brew >/dev/null 2>&1; then
        print_status "Homebrew installed"
    else
        print_warning "Homebrew not installed (recommended for easier dependency management)"
        echo "  Install from: https://brew.sh"
    fi
}

check_windows_deps() {
    print_info "Windows dependency check (manual verification required):"
    echo "  - Visual Studio 2019/2022 with C++ workload"
    echo "  - Windows SDK (usually included with Visual Studio)"
    echo "  - All required Windows libraries are system libraries"
    
    if command -v cl >/dev/null 2>&1; then
        print_status "MSVC compiler found"
    elif command -v g++ >/dev/null 2>&1; then
        print_status "MinGW-w64 compiler found"
    else
        print_warning "No suitable Windows C++ compiler found"
        echo "  Install Visual Studio with C++ workload or MinGW-w64"
    fi
}

# Perform test build
test_build() {
    echo "4. Test Build"
    echo "------------"
    
    if [[ ! -d "src" ]]; then
        print_error "Not in SystemMonitor project directory"
        echo "  Please run this script from the project root directory"
        return 1
    fi
    
    # Clean previous build
    if [[ -d "build" ]]; then
        print_info "Cleaning previous build..."
        rm -rf build
    fi
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure
    print_info "Configuring build..."
    if cmake .. >/dev/null 2>&1; then
        print_status "CMake configuration successful"
    else
        print_error "CMake configuration failed"
        echo "  Run 'cmake ..' manually to see detailed error messages"
        cd ..
        return 1
    fi
    
    # Build
    print_info "Building project..."
    if make >/dev/null 2>&1; then
        print_status "Build successful"
    else
        print_error "Build failed"
        echo "  Run 'make' manually to see detailed error messages"
        cd ..
        return 1
    fi
    
    # Test executable
    if [[ -f "SystemMonitor" ]]; then
        print_status "Executable created successfully"
        print_info "Testing executable..."
        if timeout 2s ./SystemMonitor >/dev/null 2>&1 || [[ $? -eq 124 ]]; then
            print_status "Executable runs correctly"
        else
            print_warning "Executable may have runtime issues"
        fi
    else
        print_error "Executable not created"
    fi
    
    cd ..
    echo
}

# Suggestion functions
suggest_cmake_upgrade() {
    echo "  Upgrade suggestions:"
    case $PACKAGE_MANAGER in
        "apt")
            echo "    sudo apt remove cmake"
            echo "    wget https://github.com/Kitware/CMake/releases/download/v3.22.0/cmake-3.22.0-linux-x86_64.sh"
            echo "    sudo sh cmake-3.22.0-linux-x86_64.sh --prefix=/usr/local --skip-license"
            ;;
        "brew")
            echo "    brew upgrade cmake"
            ;;
        *)
            echo "    Download from https://cmake.org/download/"
            ;;
    esac
}

suggest_cmake_install() {
    echo "  Installation suggestions:"
    case $PACKAGE_MANAGER in
        "apt")
            echo "    sudo apt update && sudo apt install cmake"
            ;;
        "yum")
            echo "    sudo yum install cmake"
            ;;
        "dnf")
            echo "    sudo dnf install cmake"
            ;;
        "pacman")
            echo "    sudo pacman -S cmake"
            ;;
        "brew")
            echo "    brew install cmake"
            ;;
        *)
            echo "    Download from https://cmake.org/download/"
            ;;
    esac
}

suggest_compiler_install() {
    echo "  Installation suggestions:"
    case $PACKAGE_MANAGER in
        "apt")
            echo "    sudo apt update && sudo apt install build-essential"
            ;;
        "yum"|"dnf")
            echo "    sudo $PACKAGE_MANAGER groupinstall \"Development Tools\""
            ;;
        "pacman")
            echo "    sudo pacman -S base-devel"
            ;;
        "brew")
            echo "    xcode-select --install"
            ;;
        *)
            echo "    Install a C++17 compatible compiler"
            ;;
    esac
}

# Generate build report
generate_report() {
    echo "5. Build Report Summary"
    echo "======================"
    
    echo "Platform: $PLATFORM"
    echo "Package Manager: $PACKAGE_MANAGER"
    
    if command -v cmake >/dev/null 2>&1; then
        echo "CMake: $(cmake --version | head -n1)"
    else
        echo "CMake: Not installed"
    fi
    
    if command -v g++ >/dev/null 2>&1; then
        echo "Compiler: $(g++ --version | head -n1)"
    elif command -v clang++ >/dev/null 2>&1; then
        echo "Compiler: $(clang++ --version | head -n1)"
    else
        echo "Compiler: Not found"
    fi
    
    if [[ -f "build/SystemMonitor" ]]; then
        echo "Build Status: ✓ SUCCESS"
        echo "Executable: build/SystemMonitor"
    else
        echo "Build Status: ✗ FAILED"
    fi
    
    echo
    echo "For detailed build instructions, see BUILD.md"
    echo "For manual troubleshooting, run individual commands from this script"
}

# Main execution
main() {
    detect_platform
    check_requirements
    check_dependencies
    test_build
    generate_report
}

# Run if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi