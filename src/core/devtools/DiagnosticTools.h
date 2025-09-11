#pragma once
#include <string>
#include <vector>

namespace TCMT {
namespace DevTools {

/**
 * 内置诊断命令行工具
 * 提供系统诊断、GPU检测、硬件状态等开发调试功能
 */
class DiagnosticTools {
public:
    static DiagnosticTools& GetInstance();
    
    /**
     * 初始化诊断工具
     */
    bool Initialize();
    
    /**
     * 启动命令行界面
     */
    void StartCommandLine();
    
    /**
     * 执行诊断命令
     */
    bool ExecuteCommand(const std::string& command, const std::vector<std::string>& args = {});
    
    /**
     * 显示帮助信息
     */
    void ShowHelp() const;
    
    /**
     * 系统信息诊断
     */
    void DiagnoseSystem();
    
    /**
     * GPU状态诊断
     */
    void DiagnoseGPU();
    
    /**
     * 硬件监控状态诊断
     */
    void DiagnoseHardware();
    
    /**
     * 内存使用诊断
     */
    void DiagnoseMemory();
    
    /**
     * 网络状态诊断
     */
    void DiagnoseNetwork();
    
    /**
     * 清理诊断工具资源
     */
    void Cleanup();

private:
    DiagnosticTools() = default;
    ~DiagnosticTools();
    
    bool m_initialized = false;
    
    /**
     * 解析命令行输入
     */
    std::pair<std::string, std::vector<std::string>> ParseCommandLine(const std::string& input);
    
    /**
     * 执行系统命令并获取输出
     */
    std::string ExecuteSystemCommand(const std::string& command);
};

} // namespace DevTools
} // namespace TCMT