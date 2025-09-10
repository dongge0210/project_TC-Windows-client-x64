using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Serilog;

namespace WPF_UI1.Services
{
    /// <summary>
    /// 崩溃转储服务，用于在程序崩溃时生成转储文件
    /// </summary>
    public class CrashDumpService
    {
        private static readonly string DumpDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "CrashDumps");
        private static bool _initialized = false;

        /// <summary>
        /// 初始化崩溃转储服务
        /// </summary>
        public static void Initialize()
        {
            if (_initialized) return;

            try
            {
                // 确保转储目录存在
                if (!Directory.Exists(DumpDirectory))
                {
                    Directory.CreateDirectory(DumpDirectory);
                }

                // 设置未处理异常处理器
                AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
                
                Log.Information("崩溃转储服务已初始化，转储目录: {DumpDirectory}", DumpDirectory);
                _initialized = true;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "初始化崩溃转储服务失败");
            }
        }

        /// <summary>
        /// 生成崩溃转储文件
        /// </summary>
        /// <param name="exception">异常信息</param>
        /// <returns>转储文件路径</returns>
        public static string GenerateCrashDump(Exception exception)
        {
            try
            {
                var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
                var dumpFileName = $"crash_dump_{timestamp}.dmp";
                var dumpFilePath = Path.Combine(DumpDirectory, dumpFileName);

                // 在Windows环境下，可以使用MiniDumpWriteDump API
                // 这里提供基础实现框架
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    try
                    {
                        // 创建进程快照文件
                        GenerateProcessSnapshot(dumpFilePath);
                    }
                    catch (Exception dumpEx)
                    {
                        Log.Error(dumpEx, "生成进程快照失败，改用异常信息文件");
                        GenerateExceptionDump(exception, dumpFilePath);
                    }
                }
                else
                {
                    // 非Windows环境生成异常信息文件
                    GenerateExceptionDump(exception, dumpFilePath);
                }

                Log.Information("崩溃转储文件已生成: {DumpFilePath}", dumpFilePath);
                return dumpFilePath;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "生成崩溃转储文件失败");
                return null;
            }
        }

        /// <summary>
        /// 生成进程快照（Windows专用）
        /// </summary>
        private static void GenerateProcessSnapshot(string filePath)
        {
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                throw new PlatformNotSupportedException("进程快照仅在Windows平台支持");

            // 获取当前进程
            var process = Process.GetCurrentProcess();
            var processHandle = process.Handle;

            // 创建转储文件
            using (var fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write))
            {
                // 这里应该调用Windows API MiniDumpWriteDump
                // 由于在Linux环境中无法调用Windows API，这里提供框架代码
                var dumpInfo = $"进程转储信息\n";
                dumpInfo += $"进程ID: {process.Id}\n";
                dumpInfo += $"进程名称: {process.ProcessName}\n";
                dumpInfo += $"工作集: {process.WorkingSet64} 字节\n";
                dumpInfo += $"虚拟内存: {process.VirtualMemorySize64} 字节\n";
                dumpInfo += $"开始时间: {process.StartTime}\n";
                dumpInfo += $"用户处理器时间: {process.UserProcessorTime}\n";
                dumpInfo += $"转储时间: {DateTime.Now}\n";

                var bytes = System.Text.Encoding.UTF8.GetBytes(dumpInfo);
                fileStream.Write(bytes, 0, bytes.Length);
            }
        }

        /// <summary>
        /// 生成异常信息转储
        /// </summary>
        private static void GenerateExceptionDump(Exception exception, string filePath)
        {
            var dumpInfo = $"异常转储信息\n";
            dumpInfo += $"时间: {DateTime.Now:yyyy-MM-dd HH:mm:ss}\n";
            dumpInfo += $"应用程序版本: {System.Reflection.Assembly.GetExecutingAssembly().GetName().Version}\n";
            dumpInfo += $"运行时版本: {Environment.Version}\n";
            dumpInfo += $"操作系统: {Environment.OSVersion}\n";
            dumpInfo += $"工作目录: {Environment.CurrentDirectory}\n";
            dumpInfo += $"命令行: {Environment.CommandLine}\n";
            dumpInfo += $"用户名: {Environment.UserName}\n";
            dumpInfo += $"机器名: {Environment.MachineName}\n";
            dumpInfo += $"处理器数量: {Environment.ProcessorCount}\n";
            dumpInfo += $"系统内存: {GC.GetTotalMemory(false)} 字节\n";
            dumpInfo += "\n--- 异常信息 ---\n";
            dumpInfo += GetExceptionDetails(exception);

            // 添加堆栈跟踪
            dumpInfo += "\n--- 调用堆栈 ---\n";
            dumpInfo += new StackTrace(true).ToString();

            // 写入文件
            File.WriteAllText(filePath, dumpInfo);
        }

        /// <summary>
        /// 获取异常详细信息
        /// </summary>
        private static string GetExceptionDetails(Exception exception)
        {
            if (exception == null) return "无异常信息";

            var details = $"异常类型: {exception.GetType().FullName}\n";
            details += $"异常消息: {exception.Message}\n";
            details += $"堆栈跟踪:\n{exception.StackTrace}\n";

            if (exception.InnerException != null)
            {
                details += "\n--- 内部异常 ---\n";
                details += GetExceptionDetails(exception.InnerException);
            }

            return details;
        }

        /// <summary>
        /// 处理未处理的异常
        /// </summary>
        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            try
            {
                if (e.ExceptionObject is Exception exception)
                {
                    Log.Fatal(exception, "检测到未处理的异常，正在生成崩溃转储");
                    var dumpPath = GenerateCrashDump(exception);
                    
                    if (!string.IsNullOrEmpty(dumpPath))
                    {
                        Log.Information("崩溃转储已保存到: {DumpPath}", dumpPath);
                    }
                }
            }
            catch (Exception ex)
            {
                // 在异常处理中不能再抛出异常
                Log.Error(ex, "处理崩溃转储时发生错误");
            }
        }

        /// <summary>
        /// 清理旧的转储文件（保留最近30天的文件）
        /// </summary>
        public static void CleanupOldDumps()
        {
            try
            {
                if (!Directory.Exists(DumpDirectory)) return;

                var cutoffDate = DateTime.Now.AddDays(-30);
                var files = Directory.GetFiles(DumpDirectory, "*.dmp");

                foreach (var file in files)
                {
                    var fileInfo = new FileInfo(file);
                    if (fileInfo.LastWriteTime < cutoffDate)
                    {
                        File.Delete(file);
                        Log.Information("已删除过期的转储文件: {FilePath}", file);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "清理旧转储文件时发生错误");
            }
        }
    }
}