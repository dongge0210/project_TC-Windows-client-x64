#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <io.h>
#include <fcntl.h>
#include <windows.h> // For MultiByteToWideChar
#include <algorithm>
#include <cctype>

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::consoleOutputEnabled = false; // Initialize console output flag

void Logger::Initialize(const std::string& logFilePath) {
    logFile.open(logFilePath, std::ios::binary | std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("无法打开日志文件");
    }

    // Write UTF-8 BOM if the file is empty
    if (logFile.tellp() == 0) {
        const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        logFile.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Set console output to UTF-8
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
}

void Logger::EnableConsoleOutput(bool enable) {
    consoleOutputEnabled = enable;
}

std::wstring Logger::ConvertToWideString(const std::string& utf8Str) {
    // Handle empty string case
    if (utf8Str.empty()) {
        return std::wstring();
    }
    
    // Get required buffer size
    int bufferSize = MultiByteToWideChar(
        CP_UTF8,                // Code page: UTF-8
        0,                      // Flags
        utf8Str.c_str(),        // Source UTF-8 string
        static_cast<int>(utf8Str.length()), // Source string length
        nullptr,                // Output buffer (null to get required size)
        0                       // Output buffer size
    );
    
    if (bufferSize == 0) {
        throw std::runtime_error("无法将UTF-8字符串转化为宽字符串");
    }
    
    // Create buffer to hold the wide string
    std::wstring wideStr(bufferSize, L'\0');
    
    // Convert the string
    if (!MultiByteToWideChar(
        CP_UTF8,                // Code page: UTF-8
        0,                      // Flags
        utf8Str.c_str(),        // Source UTF-8 string
        static_cast<int>(utf8Str.length()), // Source string length
        &wideStr[0],            // Output buffer
        bufferSize              // Output buffer size
    )) {
        throw std::runtime_error("无法将UTF-8字符串转化为宽字符串");
    }
    
    return wideStr;
}

// 新增：宽字符串转UTF-8字符串（不使用codecvt）
static std::string WideStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

// 新增：日志字符串清理，去除前后空白和不可见字符
static std::string CleanLogString(const std::string& input) {
    std::string s = input;
    // 去除前后空白
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    // 替换不可见字符为普通空格
    std::replace_if(s.begin(), s.end(), [](unsigned char ch) { return (ch == '\t' || ch == '\r' || ch == '\n' || ch == '\xa0'); }, ' ');
    return s;
}

void Logger::WriteLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (message.empty()) {
        throw std::invalid_argument("日志消息不能为空");
    }

    // 去除潜在的前导空格
    std::string trimmedMessage = message;
    while (!trimmedMessage.empty() && std::isspace(static_cast<unsigned char>(trimmedMessage.front()))) {
        trimmedMessage.erase(trimmedMessage.begin());
    }

    if (logFile.is_open()) {
        // Validate stream state
        if (!logFile.good()) {
            throw std::runtime_error("日志文件流处于无效状态");
        }

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_now = std::chrono::system_clock::to_time_t(now);

        // Use safe version of localtime
        std::tm timeinfo;
        localtime_s(&timeinfo, &time_now);

        // Construct log message
        std::stringstream ss;
        ss << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "]"
           << "[" << level << "] "
           << trimmedMessage
           << std::endl;

        std::string logEntry = ss.str();

        // Ensure buffer size is even
        if (logEntry.size() % 2 != 0) {
            logEntry += " "; // Add padding to make size even
        }

        // Write to log file
        logFile.write(logEntry.c_str(), logEntry.size());
        logFile.flush();

        // Optional console output
        if (consoleOutputEnabled) {
            std::wstring wideLogEntry = ConvertToWideString(logEntry);
            std::wcout << wideLogEntry; // Output to console only once
        }
    } else {
        throw std::runtime_error("日志文件未能打开");
    }
}

void Logger::Info(const std::string& message) {
    std::string cleanMsg = CleanLogString(message);
    WriteLog("INFO", cleanMsg);
}

void Logger::Error(const std::string& message) {
    WriteLog("ERROR", message);
}

void Logger::Warning(const std::string& message) {
    WriteLog("WARNING", message);
}

void Logger::Info(const std::wstring& message) {
    Info(WideStringToUtf8(message));
}
void Logger::Warning(const std::wstring& message) {
    Warning(WideStringToUtf8(message));
}
void Logger::Error(const std::wstring& message) {
    Error(WideStringToUtf8(message));
}

