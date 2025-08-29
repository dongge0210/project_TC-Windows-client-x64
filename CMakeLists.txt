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

# Platform-specific configurations
if(WIN32)
    target_compile_definitions(SystemMonitor PRIVATE 
        _WIN32_WINNT=0x0601
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        PLATFORM_WINDOWS=1
    )
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
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(IO_KIT IOKit)
    target_link_libraries(SystemMonitor ${CORE_FOUNDATION} ${IO_KIT})
elseif(UNIX)
    target_compile_definitions(SystemMonitor PRIVATE PLATFORM_LINUX=1)
    target_link_libraries(SystemMonitor pthread)
endif()

# Include directories
target_include_directories(SystemMonitor PRIVATE src)

# Compiler-specific flags
if(MSVC)
    target_compile_options(SystemMonitor PRIVATE /W3)
else()
    target_compile_options(SystemMonitor PRIVATE -Wall -Wextra)
endif()