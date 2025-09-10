using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Serilog;
using Microsoft.Extensions.DependencyInjection;

namespace WPF_UI1.Plugins
{
    /// <summary>
    /// 插件管理器
    /// </summary>
    public class PluginManager : IDisposable
    {
        private readonly List<IPlugin> _loadedPlugins = new List<IPlugin>();
        private readonly Dictionary<string, Assembly> _loadedAssemblies = new Dictionary<string, Assembly>();
        private readonly IServiceProvider _serviceProvider;
        private readonly ILogger _logger;
        private readonly string _pluginDirectory;
        private bool _disposed = false;

        public PluginManager(IServiceProvider serviceProvider, ILogger logger)
        {
            _serviceProvider = serviceProvider;
            _logger = logger;
            _pluginDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Plugins");
            
            // 确保插件目录存在
            if (!Directory.Exists(_pluginDirectory))
            {
                Directory.CreateDirectory(_pluginDirectory);
                _logger.Information("已创建插件目录: {PluginDirectory}", _pluginDirectory);
            }
        }

        /// <summary>
        /// 已加载的插件列表
        /// </summary>
        public IReadOnlyList<IPlugin> LoadedPlugins => _loadedPlugins.AsReadOnly();

        /// <summary>
        /// 加载所有插件
        /// </summary>
        public void LoadAllPlugins()
        {
            try
            {
                _logger.Information("开始加载插件，插件目录: {PluginDirectory}", _pluginDirectory);

                var dllFiles = Directory.GetFiles(_pluginDirectory, "*.dll", SearchOption.AllDirectories);
                
                foreach (var dllFile in dllFiles)
                {
                    try
                    {
                        LoadPluginFromAssembly(dllFile);
                    }
                    catch (Exception ex)
                    {
                        _logger.Error(ex, "加载插件失败: {DllFile}", dllFile);
                    }
                }

                _logger.Information("插件加载完成，共加载 {Count} 个插件", _loadedPlugins.Count);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "加载插件时发生错误");
            }
        }

        /// <summary>
        /// 从程序集加载插件
        /// </summary>
        /// <param name="assemblyPath">程序集路径</param>
        private void LoadPluginFromAssembly(string assemblyPath)
        {
            var assemblyName = Path.GetFileNameWithoutExtension(assemblyPath);
            
            if (_loadedAssemblies.ContainsKey(assemblyName))
            {
                _logger.Warning("程序集已加载，跳过: {AssemblyName}", assemblyName);
                return;
            }

            var assembly = Assembly.LoadFrom(assemblyPath);
            _loadedAssemblies[assemblyName] = assembly;

            var pluginTypes = assembly.GetTypes()
                .Where(t => typeof(IPlugin).IsAssignableFrom(t) && !t.IsInterface && !t.IsAbstract)
                .ToList();

            foreach (var pluginType in pluginTypes)
            {
                try
                {
                    var plugin = CreatePluginInstance(pluginType);
                    if (plugin != null)
                    {
                        _loadedPlugins.Add(plugin);
                        _logger.Information("已加载插件: {PluginName} v{Version} by {Author}", 
                            plugin.Name, plugin.Version, plugin.Author);
                    }
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "创建插件实例失败: {PluginType}", pluginType.FullName);
                }
            }
        }

        /// <summary>
        /// 创建插件实例
        /// </summary>
        /// <param name="pluginType">插件类型</param>
        /// <returns>插件实例</returns>
        private IPlugin CreatePluginInstance(Type pluginType)
        {
            try
            {
                // 尝试通过依赖注入创建实例
                var plugin = ActivatorUtilities.CreateInstance(_serviceProvider, pluginType) as IPlugin;
                
                if (plugin == null)
                {
                    // 回退到默认构造函数
                    plugin = Activator.CreateInstance(pluginType) as IPlugin;
                }

                return plugin;
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "创建插件实例失败: {PluginType}", pluginType.FullName);
                return null;
            }
        }

        /// <summary>
        /// 初始化所有插件
        /// </summary>
        public void InitializeAllPlugins()
        {
            foreach (var plugin in _loadedPlugins)
            {
                try
                {
                    if (plugin.IsEnabled)
                    {
                        var context = new PluginContext(_serviceProvider, _logger.ForContext("Plugin", plugin.Name));
                        plugin.Initialize(context);
                        _logger.Information("插件已初始化: {PluginName}", plugin.Name);
                    }
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "初始化插件失败: {PluginName}", plugin.Name);
                }
            }
        }

        /// <summary>
        /// 启动所有插件
        /// </summary>
        public void StartAllPlugins()
        {
            foreach (var plugin in _loadedPlugins)
            {
                try
                {
                    if (plugin.IsEnabled)
                    {
                        plugin.Start();
                        _logger.Information("插件已启动: {PluginName}", plugin.Name);
                    }
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "启动插件失败: {PluginName}", plugin.Name);
                }
            }
        }

        /// <summary>
        /// 停止所有插件
        /// </summary>
        public void StopAllPlugins()
        {
            foreach (var plugin in _loadedPlugins)
            {
                try
                {
                    plugin.Stop();
                    _logger.Information("插件已停止: {PluginName}", plugin.Name);
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "停止插件失败: {PluginName}", plugin.Name);
                }
            }
        }

        /// <summary>
        /// 获取指定类型的插件
        /// </summary>
        /// <typeparam name="T">插件类型</typeparam>
        /// <returns>插件列表</returns>
        public List<T> GetPlugins<T>() where T : class, IPlugin
        {
            return _loadedPlugins.OfType<T>().ToList();
        }

        /// <summary>
        /// 获取插件
        /// </summary>
        /// <param name="name">插件名称</param>
        /// <returns>插件实例</returns>
        public IPlugin GetPlugin(string name)
        {
            return _loadedPlugins.FirstOrDefault(p => p.Name == name);
        }

        /// <summary>
        /// 启用插件
        /// </summary>
        /// <param name="pluginName">插件名称</param>
        public void EnablePlugin(string pluginName)
        {
            var plugin = GetPlugin(pluginName);
            if (plugin != null && !plugin.IsEnabled)
            {
                plugin.IsEnabled = true;
                try
                {
                    var context = new PluginContext(_serviceProvider, _logger.ForContext("Plugin", plugin.Name));
                    plugin.Initialize(context);
                    plugin.Start();
                    _logger.Information("插件已启用: {PluginName}", pluginName);
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "启用插件失败: {PluginName}", pluginName);
                    plugin.IsEnabled = false;
                }
            }
        }

        /// <summary>
        /// 禁用插件
        /// </summary>
        /// <param name="pluginName">插件名称</param>
        public void DisablePlugin(string pluginName)
        {
            var plugin = GetPlugin(pluginName);
            if (plugin != null && plugin.IsEnabled)
            {
                try
                {
                    plugin.Stop();
                    plugin.IsEnabled = false;
                    _logger.Information("插件已禁用: {PluginName}", pluginName);
                }
                catch (Exception ex)
                {
                    _logger.Error(ex, "禁用插件失败: {PluginName}", pluginName);
                }
            }
        }

        /// <summary>
        /// 释放资源
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                StopAllPlugins();
                
                foreach (var plugin in _loadedPlugins)
                {
                    try
                    {
                        plugin.Dispose();
                    }
                    catch (Exception ex)
                    {
                        _logger.Error(ex, "释放插件资源失败: {PluginName}", plugin.Name);
                    }
                }

                _loadedPlugins.Clear();
                _loadedAssemblies.Clear();
                _disposed = true;
            }
        }
    }

    /// <summary>
    /// 插件上下文实现
    /// </summary>
    internal class PluginContext : IPluginContext
    {
        private readonly Dictionary<Type, List<Delegate>> _messageHandlers = new Dictionary<Type, List<Delegate>>();

        public PluginContext(IServiceProvider serviceProvider, ILogger logger)
        {
            ServiceProvider = serviceProvider;
            Logger = logger;
            Configuration = new Dictionary<string, object>();
        }

        public IServiceProvider ServiceProvider { get; }
        public IDictionary<string, object> Configuration { get; }
        public ILogger Logger { get; }

        public T GetService<T>()
        {
            return ServiceProvider.GetService<T>();
        }

        public void SendMessage(IPluginMessage message)
        {
            var messageType = message.GetType();
            if (_messageHandlers.ContainsKey(messageType))
            {
                foreach (var handler in _messageHandlers[messageType])
                {
                    try
                    {
                        handler.DynamicInvoke(message);
                    }
                    catch (Exception ex)
                    {
                        Logger.Error(ex, "处理插件消息失败: {MessageType}", messageType.Name);
                    }
                }
            }
        }

        public void SubscribeMessage<T>(Action<T> handler) where T : IPluginMessage
        {
            var messageType = typeof(T);
            if (!_messageHandlers.ContainsKey(messageType))
            {
                _messageHandlers[messageType] = new List<Delegate>();
            }
            _messageHandlers[messageType].Add(handler);
        }
    }
}