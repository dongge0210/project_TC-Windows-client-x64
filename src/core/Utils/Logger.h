#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <string>
#include <fstream>

class Logger {
private:
    struct LogItem {
        std::string level;
        std::string message;
    };

    static std::ofstream logFile;
    static std::mutex queueMutex;
    static std::condition_variable queueCV;
    static std::queue<LogItem> logQueue;
    static std::thread logThread;
    static std::atomic<bool> running;
    static bool consoleOutputEnabled;

    static void LogThreadFunc();
    static void EnqueueLog(const std::string& level, const std::string& message);
    static std::wstring ConvertToWideString(const std::string& utf8Str);

public:
    static void Initialize(const std::string& logFilePath);
    static void Shutdown();
    static void EnableConsoleOutput(bool enable);
    static void Info(const std::string& message);
    static void Error(const std::string& message);
    static void Warning(const std::string& message);

    static void Info(const std::wstring& message);
    static void Warning(const std::wstring& message);
    static void Error(const std::wstring& message);

    static void InfoUtf8(const std::string& message);
    static void ErrorUtf8(const std::string& message);
};
