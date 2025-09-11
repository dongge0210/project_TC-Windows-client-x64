#pragma once
#include <string>
#include <functional>

namespace TCMT {
namespace Services {

/**
 * 系统服务接口 - 处理系统级功能
 */
class ISystemService {
public:
    virtual ~ISystemService() = default;
    
    /**
     * 初始化服务
     */
    virtual bool Initialize() = 0;
    
    /**
     * 启动服务
     */
    virtual bool Start() = 0;
    
    /**
     * 停止服务
     */
    virtual void Stop() = 0;
    
    /**
     * 检查服务状态
     */
    virtual bool IsRunning() const = 0;
    
    /**
     * 获取服务名称
     */
    virtual std::string GetServiceName() const = 0;
};

/**
 * 监控服务接口 - 处理硬件监控
 */
class IMonitoringService : public ISystemService {
public:
    using DataUpdateCallback = std::function<void()>;
    
    /**
     * 设置数据更新回调
     */
    virtual void SetDataUpdateCallback(DataUpdateCallback callback) = 0;
    
    /**
     * 获取更新间隔（毫秒）
     */
    virtual int GetUpdateInterval() const = 0;
    
    /**
     * 设置更新间隔（毫秒）
     */
    virtual void SetUpdateInterval(int intervalMs) = 0;
    
    /**
     * 强制更新数据
     */
    virtual void ForceUpdate() = 0;
};

/**
 * 热键服务接口 - 处理全局热键
 */
class IHotkeyService : public ISystemService {
public:
    using HotkeyCallback = std::function<void(int hotkeyId)>;
    
    /**
     * 注册热键
     */
    virtual bool RegisterHotkey(int id, unsigned int modifiers, unsigned int keyCode, HotkeyCallback callback) = 0;
    
    /**
     * 取消注册热键
     */
    virtual bool UnregisterHotkey(int id) = 0;
    
    /**
     * 取消注册所有热键
     */
    virtual void UnregisterAllHotkeys() = 0;
};

/**
 * 系统托盘服务接口 - 处理系统托盘
 */
class ISystemTrayService : public ISystemService {
public:
    using TrayCallback = std::function<void(int action)>;
    
    /**
     * 设置托盘图标
     */
    virtual bool SetTrayIcon(const std::wstring& iconPath) = 0;
    
    /**
     * 设置托盘提示文本
     */
    virtual bool SetTrayTooltip(const std::wstring& tooltip) = 0;
    
    /**
     * 显示托盘通知
     */
    virtual bool ShowNotification(const std::wstring& title, const std::wstring& message) = 0;
    
    /**
     * 设置托盘回调
     */
    virtual void SetTrayCallback(TrayCallback callback) = 0;
};

} // namespace Services
} // namespace TCMT