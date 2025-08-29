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
file(GLOB_RECURSE SOURCES 
    "src/main.cpp"
    "src/core/*.cpp"
    "src/core/*.h"
)

# Platform-specific exclusions
if(NOT WIN32)
    # Exclude Windows-specific files on non-Windows platforms
    list(FILTER SOURCES EXCLUDE REGEX ".*[Ww]in.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*[Ww]mi.*")
endif()

add_executable(SystemMonitor ${SOURCES})

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