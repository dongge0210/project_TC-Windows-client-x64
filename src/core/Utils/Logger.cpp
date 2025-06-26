#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <algorithm>
#include <cctype>

// 静态成员初始化
std::ofstream Logger::logFile;
std::mutex Logger::queueMutex;
std::condition_variable Logger::queueCV;
std::queue<Logger::LogItem> Logger::logQueue;
std::thread Logger::logThread;
std::atomic<bool> Logger::running = false;
bool Logger::consoleOutputEnabled = false;

// 辅助：宽字符串转UTF-8
static std::string WideStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

// 辅助：UTF-8转宽字符串
std::wstring Logger::ConvertToWideString(const std::string& utf8Str) {
    if (utf8Str.empty()) return std::wstring();
    int bufferSize = MultiByteToWideChar(
        CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.length(), nullptr, 0);
    if (bufferSize == 0) throw std::runtime_error("无法将UTF-8字符串转化为宽字符串");
    std::wstring wideStr(bufferSize, L'\0');
    if (!MultiByteToWideChar(
        CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.length(), &wideStr[0], bufferSize))
        throw std::runtime_error("无法将UTF-8字符串转化为宽字符串");
    return wideStr;
}

// 日志线程主循环
void Logger::LogThreadFunc() {
    while (running || !logQueue.empty()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [] { return !logQueue.empty() || !running; });
        while (!logQueue.empty()) {
            auto item = logQueue.front();
            logQueue.pop();
            lock.unlock();

            // 格式化日志
            auto now = std::chrono::system_clock::now();
            auto time_now = std::chrono::system_clock::to_time_t(now);
            std::tm timeinfo;
            localtime_s(&timeinfo, &time_now);
            std::stringstream ss;
            ss << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "]"
                << "[" << item.level << "] " << item.message << std::endl;
            std::string logEntry = ss.str();

            if (logFile.is_open()) {
                logFile.write(logEntry.c_str(), logEntry.size());
                logFile.flush();
            }
            if (consoleOutputEnabled) {
                std::wstring wideLogEntry = ConvertToWideString(logEntry);
                std::wcout << wideLogEntry;
            }
            lock.lock();
        }
    }
}

// 日志入队
void Logger::EnqueueLog(const std::string& level, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.push({ level, message });
    }
    queueCV.notify_one();
}

// 初始化（启动线程）
void Logger::Initialize(const std::string& logFilePath) {
    logFile.open(logFilePath, std::ios::binary | std::ios::app);
    if (!logFile.is_open()) throw std::runtime_error("无法打开日志文件");
    if (logFile.tellp() == 0) {
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        logFile.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    running = true;
    logThread = std::thread(LogThreadFunc);
}

// 关闭（优雅退出线程）
void Logger::Shutdown() {
    running = false;
    queueCV.notify_all();
    if (logThread.joinable()) logThread.join();
    if (logFile.is_open()) logFile.close();
}

// 控制台输出开关
void Logger::EnableConsoleOutput(bool enable) {
    consoleOutputEnabled = enable;
}

// 公开接口
void Logger::Info(const std::string& message) { EnqueueLog("INFO", message); }
void Logger::Warning(const std::string& message) { EnqueueLog("WARNING", message); }
void Logger::Error(const std::string& message) { EnqueueLog("ERROR", message); }

void Logger::Info(const std::wstring& message) { Info(WideStringToUtf8(message)); }
void Logger::Warning(const std::wstring& message) { Warning(WideStringToUtf8(message)); }
void Logger::Error(const std::wstring& message) { Error(WideStringToUtf8(message)); }

void Logger::InfoUtf8(const std::string &message) {
    // Replace std::wstring_convert usage with MultiByteToWideChar
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), (int)message.size(), nullptr, 0);
    if (sizeNeeded <= 0) {
        wprintf(L"[INFO] Conversion error\n");
        return;
    }
    std::wstring wmsg(sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), (int)message.size(), &wmsg[0], sizeNeeded);
    wprintf(L"[INFO] %ls\n", wmsg.c_str());
}

void Logger::ErrorUtf8(const std::string &message) {
    // Replace std::wstring_convert usage with MultiByteToWideChar
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), (int)message.size(), nullptr, 0);
    if (sizeNeeded <= 0) {
        wprintf(L"[ERROR] Conversion error\n");
        return;
    }
    std::wstring wmsg(sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), (int)message.size(), &wmsg[0], sizeNeeded);
    wprintf(L"[ERROR] %ls\n", wmsg.c_str());
}


