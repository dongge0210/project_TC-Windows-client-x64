#pragma once

#include <string>
#include <vector>
#include <utility> // For std::pair
#include <memory>  // For std::unique_ptr

/**
 * @brief LibreHardwareMonitor桥接类
 *
 * 提供对LibreHardwareMonitor库的访问功能，用于获取系统温度和其他硬件信息。
 * 使用PIMPL设计模式将CLR/.NET代码与C++代码隔离，提高兼容性。
 */
class LibreHardwareMonitorBridge {
private:
    // 前置声明，用于隐藏实现细节
    class CLRBridgeImpl;

    // 指向实现的智能指针
    static std::unique_ptr<CLRBridgeImpl> impl;

    // 初始化状态标志
    static bool initialized;

    // 保存初始化错误信息
    static std::string lastError;

    // DLL路径常量
    static const char* const DEFAULT_DLL_PATH;

public:
    /**
     * @brief 初始化LibreHardwareMonitor
     *
     * 尝试加载LibreHardwareMonitor库并初始化硬件监控功能。
     * 如果已经初始化，则不进行任何操作。
     *
     * @param dllPath 可选参数，指定DLL的路径，如果为空则使用默认路径
     */
    static void Initialize(const std::string& dllPath = "");

    /**
     * @brief 清理资源
     *
     * 释放所有分配的资源并关闭硬件监控功能。
     */
    static void Cleanup();

    /**
     * @brief 获取温度数据
     *
     * @return 包含传感器名称和温度值(°C)的向量
     */
    static std::vector<std::pair<std::string, double>> GetTemperatures();

    /**
     * @brief 获取初始化状态
     *
     * @return 如果成功初始化则返回true，否则返回false
     */
    static bool IsInitialized() { return initialized; }

    /**
     * @brief 获取上一次初始化错误
     *
     * @return 错误信息字符串
     */
    static std::string GetLastError() { return lastError; }

    /**
     * @brief 获取.NET运行时版本
     *
     * @return .NET运行时版本字符串
     */
    static std::string GetDotNetRuntimeVersion();
};
