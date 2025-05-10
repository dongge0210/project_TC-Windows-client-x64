#pragma once

#include <string>
#include <vector>
#include <utility> // For std::pair
#include <memory>  // For std::unique_ptr
#include <msclr/gcroot.h>

// Forward declare .NET types
namespace LibreHardwareMonitor { namespace Hardware {
    ref class Computer;
    ref class UpdateVisitor;
    interface class IVisitor; // 修正：interface class 而不是 ref class
} } // 注意：补全命名空间括号

/**
 * @brief LibreHardwareMonitor桥接类
 *
 * 提供对LibreHardwareMonitor库的访问功能，用于获取系统温度和其他硬件信息。
 * 使用PIMPL设计模式将CLR/.NET代码与C++代码隔离，提高兼容性。
 */
class LibreHardwareMonitorBridge {
public:
    /**
     * @brief 初始化LibreHardwareMonitor
     *
     * 尝试加载LibreHardwareMonitor库并初始化硬件监控功能。
     * 如果已经初始化，则不进行任何操作。
     */
    static void Initialize();

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
     * @brief 获取CPU功耗
     *
     * @return CPU功耗值(瓦特)
     */
    static double GetCpuPower();

    /**
     * @brief 获取GPU功耗
     *
     * @return GPU功耗值(瓦特)
     */
    static double GetGpuPower();

    /**
     * @brief 获取总功耗
     *
     * @return 总功耗值(瓦特)
     */
    static double GetTotalPower();

    ref class UpdateVisitor;

private:
    // 初始化状态标志
    static bool initialized;

    // .NET对象的托管指针
    static msclr::gcroot<LibreHardwareMonitor::Hardware::Computer^> computer;
    // Use IVisitor^ for compatibility with Accept()
    static msclr::gcroot<LibreHardwareMonitor::Hardware::IVisitor^> visitor;
};
