cmake_minimum_required(VERSION 3.16)

project(SystemMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(TARGET_PLATFORM "WINDOWS")
elseif(APPLE)
    set(TARGET_PLATFORM "MACOS")
elseif(UNIX)
    set(TARGET_PLATFORM "LINUX")
endif()

message(STATUS "Building for platform: ${TARGET_PLATFORM}")

# Source files
set(CORE_SOURCES
    "src/main.cpp"
    "src/core/os/OSInfo.cpp"
    "src/core/memory/MemoryInfo.cpp"
    "src/core/Utils/PlatformUtils.cpp"
)

# Platform-specific CPU implementation
if(WIN32)
    list(APPEND CORE_SOURCES "src/core/cpu/CpuInfo.cpp")
else()
    list(APPEND CORE_SOURCES "src/core/cpu/CpuInfo_CrossPlatform.cpp")
endif()

# Add Logger and TimeUtils only for Windows (they have Windows dependencies)
if(WIN32)
    list(APPEND CORE_SOURCES
        "src/core/Utils/Logger.cpp"
        "src/core/Utils/TimeUtils.cpp"
    )
endif()

# Platform-specific sources
if(WIN32)
    list(APPEND CORE_SOURCES
        "src/core/cpu/CpuInfo.cpp"
        "src/core/gpu/GpuInfo.cpp"
        "src/core/disk/DiskInfo.cpp"
        "src/core/network/NetworkAdapter.cpp"
        "src/core/temperature/TemperatureWrapper.cpp"
        "src/core/temperature/LibreHardwareMonitorBridge.cpp"
        "src/core/DataStruct/SharedMemoryManager.cpp"
        "src/core/DataStruct/Producer.cpp"
        "src/core/Utils/WinUtils.cpp"
        "src/core/Utils/WMIManager.cpp"
        "src/core/Utils/ComInitializationHelper.cpp"
        "src/core/Utils/LibreHardwareMonitorBridge.cpp"
    )
endif()

message(STATUS "Final sources: ${CORE_SOURCES}")

add_executable(SystemMonitor ${CORE_SOURCES})

# Platform-specific configurations and dependency handling
if(WIN32)
    target_compile_definitions(SystemMonitor PRIVATE 
        _WIN32_WINNT=0x0601
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        PLATFORM_WINDOWS=1
    )
    
    # Windows system libraries
    target_link_libraries(SystemMonitor 
        kernel32 user32 advapi32 shell32 ole32 oleaut32 uuid
        wbemuuid pdh iphlpapi ws2_32
    )
    
    # Set stack size for Windows
    set_target_properties(SystemMonitor PROPERTIES
        LINK_FLAGS "/STACK:8388608"
    )
    
elseif(APPLE)
    target_compile_definitions(SystemMonitor PRIVATE PLATFORM_MACOS=1)
    
    # Find required macOS frameworks
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    find_library(IO_KIT IOKit REQUIRED)
    
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "CoreFoundation framework not found - required for macOS build")
    endif()
    
    if(NOT IO_KIT)
        message(FATAL_ERROR "IOKit framework not found - required for macOS build")
    endif()
    
    target_link_libraries(SystemMonitor ${CORE_FOUNDATION} ${IO_KIT})
    message(STATUS "Found macOS frameworks: CoreFoundation, IOKit")
    
elseif(UNIX)
    target_compile_definitions(SystemMonitor PRIVATE PLATFORM_LINUX=1)
    
    # Find pthread (usually available on all Linux systems)
    find_package(Threads REQUIRED)
    target_link_libraries(SystemMonitor Threads::Threads)
    
    # Optional: Check for additional system libraries that might be useful
    # Check for librt (real-time extensions) - often needed for timing functions
    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        target_link_libraries(SystemMonitor ${RT_LIBRARY})
        message(STATUS "Found librt: ${RT_LIBRARY}")
    else()
        message(WARNING "librt not found - some timing functions may not be available")
    endif()
    
    message(STATUS "Linux dependencies configured")
endif()

# Include directories
target_include_directories(SystemMonitor PRIVATE src)

# Compiler-specific flags with better cross-platform support
if(MSVC)
    target_compile_options(SystemMonitor PRIVATE /W3)
    # Suppress some common Windows warnings
    target_compile_definitions(SystemMonitor PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
else()
    target_compile_options(SystemMonitor PRIVATE -Wall -Wextra -Werror)
    
    # Platform-specific compiler flags
    if(APPLE)
        target_compile_options(SystemMonitor PRIVATE -Wno-deprecated-declarations)
    endif()
endif()

# Display build configuration summary
message(STATUS "Build configuration:")
message(STATUS "  Platform: ${TARGET_PLATFORM}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")

# Add install target for better deployment
install(TARGETS SystemMonitor
    RUNTIME DESTINATION bin
    COMPONENT applications
)