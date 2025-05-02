#include "stdafx.h"  // 放在最前面
#include <QtWidgets/QApplication>
#include "QtWidgetsTCmonitor.h"
#include "../src/core/utils/Logger.h"
#include "../src/core/DataStruct/SharedMemoryManager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Initialize logging
    Logger::Initialize("qt_monitor.log");
    Logger::EnableConsoleOutput(true);
    Logger::Info("Qt 系统监视器启动");
    
    // Try to connect to existing shared memory first
    // The main process will create the shared memory, we just connect to it
    Logger::Info("尝试连接到共享内存...");
    if (!SharedMemoryManager::InitSharedMemory()) {
        Logger::Warning("无法连接到共享内存: " + SharedMemoryManager::GetSharedMemoryError());
        Logger::Info("UI 将继续运行，但无法显示系统数据");
    } else {
        Logger::Info("成功连接到共享内存");
    }
    
    // Create and show main window
    QtWidgetsTCmonitor w;
    w.show();
    
    int result = a.exec();
    
    // Clean up resources
    SharedMemoryManager::CleanupSharedMemory();
    Logger::Info("Qt 系统监视器关闭");
    
    return result;
}
