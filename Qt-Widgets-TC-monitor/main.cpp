#include "stdafx.h"  // 放在最前面
#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>  // 添加QML引擎
//#include <QQuickStyle>           // 可选样式
#include <QDebug> // 显式包含QDebug，解决QDebug相关编译错误
#include "../src/core/utils/Logger.h"
#include "../src/core/DataStruct/SharedMemoryManager.h"
#include "../src/core/disk/diskInfo.h"
#include "../src/core/Utils/WinUtils.h" // 路径区分大小写，确保Utils大写

int main(int argc, char* argv[]) {
    try
    {
        QApplication a(argc, argv);
        qputenv("QT_DEBUG_PLUGINS", "1");
        
        Logger::Initialize("qt_monitor.log");
        Logger::EnableConsoleOutput(true);
        Logger::Info("Qt 系统监视器启动");

        QQmlApplicationEngine engine;
        qDebug() << "QML load status:" << engine.rootObjects().isEmpty();

        Logger::Info("尝试连接到共享内存...");
        if (!SharedMemoryManager::InitSharedMemory()) {
            Logger::Warning("无法连接到共享内存: " + SharedMemoryManager::GetSharedMemoryError());
            Logger::Info("UI 将继续运行，但无法显示系统数据");
        }
        else {
            Logger::Info("成功连接到共享内存");
        }

        //engine.setOutputWarningsToStandardError(true); // 可选
        //engine.load(QUrl("qrc:/app.qml"));
        engine.addImportPath("F:/Win_x64-10.lastest-sysMonitor/Qt-Widgets-TC-monitor/Project1-qml/Project1Content");
        engine.load(QUrl::fromLocalFile("F:/Win_x64-10.lastest-sysMonitor/Qt-Widgets-TC-monitor/Project1-qml/Project1Content/App.qml"));

        if (engine.rootObjects().isEmpty()) {
            qCritical() << "QML界面加载失败!";
            return -1;
        }

        int result = a.exec();

        SharedMemoryManager::CleanupSharedMemory();
        Logger::Info("Qt 系统监视器关闭");

        return result;
    }
    catch (const std::exception& ex) {
        qCritical() << "捕获到 std::exception:" << ex.what();
        return -1;
    }
    catch (...) {
        qCritical() << "捕获到未知异常";
        return -1;
    }
}