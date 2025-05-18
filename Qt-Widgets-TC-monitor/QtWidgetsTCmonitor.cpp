#include "stdafx.h"
#include "QtWidgetsTCmonitor.h"
#include "ui_QtWidgetsTCmonitor.h"  // Auto-generated UI file
#include <windows.h>
#include "../src/core/DataStruct/SharedMemoryManager.h"
#include "../src/core/Utils/WinUtils.h"  // Include WinUtils for string conversion

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtGui/QPainter>
#include <QTimer>
#include <QMainWindow>
#include <QObject>
#include <QtWidgets>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <map>       // For std::map
#include <string>    // For std::string
#include <QLabel>    // For QLabel
#include <queue>
#include <sstream>
#include <iomanip>
#include <QString>
#include <QDebug> // 显式包含QDebug，解决QDebug相关编译错误

#include "QtWidgetsTCmonitor.h" // Ensure MAX_DATA_POINTS is available

// Helper function to check if a wchar_t array is null-terminated within a given max length
static bool isWCharArrayNullTerminated(const wchar_t* arr, size_t maxLen) {
    if (!arr) return false;
    for (size_t i = 0; i < maxLen; ++i) {
        if (arr[i] == L'\0') return true;
    }
    return false;
}

QtWidgetsTCmonitor::QtWidgetsTCmonitor(QWidget* parent)
    : QMainWindow(parent)
{
    ui = new Ui_QtWidgetsTCmonitorClass();
    ui->setupUi(this);

    // 显式调用，确保界面元素已初始化
    setupUI();

    // Set window properties
    setWindowTitle(tr("系统硬件监视器"));
    resize(800, 600);

    // Try to connect to shared memory
    if (!SharedMemoryManager::IsSharedMemoryInitialized()) {
        if (!SharedMemoryManager::InitSharedMemory()) {
            QMessageBox::warning(this, tr("警告"), 
                tr("无法连接到共享内存，系统数据将不会更新。\n错误: %1")
                .arg(QString::fromStdString(SharedMemoryManager::GetSharedMemoryError())));
        }
    }

    // Set up update timer
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &QtWidgetsTCmonitor::updateFromSharedMemory);
    updateTimer->start(1000); // Update once per second
}

QtWidgetsTCmonitor::~QtWidgetsTCmonitor()
{
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    delete ui; // 释放ui
}

void QtWidgetsTCmonitor::setupUI()
{
    // Create main scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    setCentralWidget(scrollArea);

    // Create main container
    QWidget* container = new QWidget(scrollArea);
    scrollArea->setWidget(container);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(container);

    // Create sections
    createCpuSection();
    createMemorySection();
    createGpuSection();
    createTemperatureSection();
    createDiskSection();
    createNetworkSection();

    // Add to main layout
    mainLayout->addWidget(cpuGroupBox);
    mainLayout->addWidget(memoryGroupBox);
    mainLayout->addWidget(gpuGroupBox);
    mainLayout->addWidget(temperatureGroupBox);
    mainLayout->addWidget(diskGroupBox);
    mainLayout->addStretch();
    mainLayout->addWidget(networkGroupBox);

    // 在主布局最后添加总功率显示
    if (!infoLabels.contains("totalPower")) {
        infoLabels["totalPower"] = new QLabel(this);
        QHBoxLayout* powerLayout = new QHBoxLayout();
        powerLayout->addWidget(new QLabel(tr("整机功率:"), this));
        powerLayout->addWidget(infoLabels["totalPower"]);
        mainLayout->addLayout(powerLayout);
    }
    if (!infoLabels.contains("motherboardName")) {
        infoLabels["motherboardName"] = new QLabel(this);
        QHBoxLayout* mbLayout = new QHBoxLayout();
        mbLayout->addWidget(new QLabel(tr("主板名称:"), this));
        mbLayout->addWidget(infoLabels["motherboardName"]);
        mainLayout->addLayout(mbLayout);
    }
    if (!infoLabels.contains("deviceName")) {
        infoLabels["deviceName"] = new QLabel(this);
        QHBoxLayout* devLayout = new QHBoxLayout();
        devLayout->addWidget(new QLabel(tr("设备名称:"), this));
        devLayout->addWidget(infoLabels["deviceName"]);
        mainLayout->addLayout(devLayout);
    }
    if (!infoLabels.contains("osVersion")) {
        infoLabels["osVersion"] = new QLabel(this);
        QHBoxLayout* osLayout = new QHBoxLayout();
        osLayout->addWidget(new QLabel(tr("系统版本:"), this));
        osLayout->addWidget(infoLabels["osVersion"]);
        mainLayout->addLayout(osLayout);
    }
}

void QtWidgetsTCmonitor::createCpuSection()
{
    cpuGroupBox = new QGroupBox(tr("处理器信息"), this);
    QGridLayout* layout = new QGridLayout(cpuGroupBox);

    int row = 0;
    layout->addWidget(new QLabel(tr("名称:"), this), row, 0);
    infoLabels["cpuName"] = new QLabel(this);
    layout->addWidget(infoLabels["cpuName"], row++, 1);

    layout->addWidget(new QLabel(tr("物理核心:"), this), row, 0);
    infoLabels["physicalCores"] = new QLabel(this);
    layout->addWidget(infoLabels["physicalCores"], row++, 1);

    layout->addWidget(new QLabel(tr("逻辑核心:"), this), row, 0);
    infoLabels["logicalCores"] = new QLabel(this);
    layout->addWidget(infoLabels["logicalCores"], row++, 1);

    layout->addWidget(new QLabel(tr("性能核心:"), this), row, 0);
    infoLabels["performanceCores"] = new QLabel(this);
    layout->addWidget(infoLabels["performanceCores"], row++, 1);

    layout->addWidget(new QLabel(tr("能效核心:"), this), row, 0);
    infoLabels["efficiencyCores"] = new QLabel(this);
    layout->addWidget(infoLabels["efficiencyCores"], row++, 1);

    layout->addWidget(new QLabel(tr("CPU使用率:"), this), row, 0);
    infoLabels["cpuUsage"] = new QLabel(this);
    layout->addWidget(infoLabels["cpuUsage"], row++, 1);

    layout->addWidget(new QLabel(tr("CPU功率:"), this), row, 0);
    infoLabels["cpuPower"] = new QLabel(this);
    layout->addWidget(infoLabels["cpuPower"], row++, 1);

    layout->addWidget(new QLabel(tr("超线程:"), this), row, 0);
    infoLabels["hyperThreading"] = new QLabel(this);
    layout->addWidget(infoLabels["hyperThreading"], row++, 1);

    layout->addWidget(new QLabel(tr("虚拟化:"), this), row, 0);
    infoLabels["virtualization"] = new QLabel(this);
    layout->addWidget(infoLabels["virtualization"], row++, 1);
}

void QtWidgetsTCmonitor::createMemorySection()
{
    memoryGroupBox = new QGroupBox(tr("内存信息"), this);
    QGridLayout* layout = new QGridLayout(memoryGroupBox);

    int row = 0;
    layout->addWidget(new QLabel(tr("总内存:"), this), row, 0);
    infoLabels["totalMemory"] = new QLabel(this);
    layout->addWidget(infoLabels["totalMemory"], row++, 1);

    layout->addWidget(new QLabel(tr("已用内存:"), this), row, 0);
    infoLabels["usedMemory"] = new QLabel(this);
    layout->addWidget(infoLabels["usedMemory"], row++, 1);

    layout->addWidget(new QLabel(tr("可用内存:"), this), row, 0);
    infoLabels["availableMemory"] = new QLabel(this);
    layout->addWidget(infoLabels["availableMemory"], row++, 1);

    layout->addWidget(new QLabel(tr("内存频率:"), this), row, 0);
    infoLabels["memoryFrequency"] = new QLabel(this);
    layout->addWidget(infoLabels["memoryFrequency"], row++, 1);

    layout->addWidget(new QLabel(tr("内存使用率:"), this), row, 0);
    infoLabels["memoryUsage"] = new QLabel(this);
    layout->addWidget(infoLabels["memoryUsage"], row++, 1);
}

void QtWidgetsTCmonitor::createGpuSection()
{
    gpuGroupBox = new QGroupBox(tr("显卡信息"), this);
    QGridLayout* layout = new QGridLayout(gpuGroupBox);

    int row = 0;

    // 添加 GPU 选择下拉框 - 在第一行
    layout->addWidget(new QLabel(tr("选择显卡:"), this), row, 0);
    gpuSelector = new QComboBox(this);
    // 添加选择改变时的槽函数
    connect(gpuSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QtWidgetsTCmonitor::onGpuSelectionChanged);
    layout->addWidget(gpuSelector, row++, 1);

    // 原有的 GPU 信息布局继续
    layout->addWidget(new QLabel(tr("名称:"), this), row, 0);
    infoLabels["gpuName"] = new QLabel(this);
    layout->addWidget(infoLabels["gpuName"], row++, 1);

    layout->addWidget(new QLabel(tr("专用显存:"), this), row, 0);
    infoLabels["gpuVram"] = new QLabel(this);                
    layout->addWidget(infoLabels["gpuVram"], row++, 1);       

    layout->addWidget(new QLabel(tr("共享内存:"), this), row, 0);
    infoLabels["gpuSharedMem"] = new QLabel(this);
    layout->addWidget(infoLabels["gpuSharedMem"], row++, 1);

    layout->addWidget(new QLabel(tr("核心频率:"), this), row, 0);
    infoLabels["gpuCoreFreq"] = new QLabel(this);
    layout->addWidget(infoLabels["gpuCoreFreq"], row++, 1);

    layout->addWidget(new QLabel(tr("GPU功率:"), this), row, 0);
    infoLabels["gpuPower"] = new QLabel(this);
    layout->addWidget(infoLabels["gpuPower"], row++, 1);
}

void QtWidgetsTCmonitor::createTemperatureSection()
{
    temperatureGroupBox = new QGroupBox(tr("温度监控"), this);
    QVBoxLayout* layout = new QVBoxLayout(temperatureGroupBox);

    // Temperature labels
    QGridLayout* tempLabelsLayout = new QGridLayout();
    tempLabelsLayout->addWidget(new QLabel(tr("CPU温度:"), this), 0, 0);
    infoLabels["cpuTemp"] = new QLabel(this);
    tempLabelsLayout->addWidget(infoLabels["cpuTemp"], 0, 1);

    tempLabelsLayout->addWidget(new QLabel(tr("GPU温度:"), this), 1, 0);
    infoLabels["gpuTemp"] = new QLabel(this);
    tempLabelsLayout->addWidget(infoLabels["gpuTemp"], 1, 1);

    layout->addLayout(tempLabelsLayout);

    // Create charts
    // CPU temperature chart
    cpuTempChart = new QChart();
    cpuTempChart->setTitle(tr("CPU温度历史"));
    cpuTempSeries = new QLineSeries();
    cpuTempChart->addSeries(cpuTempSeries);

    // Set up axes
    QValueAxis* axisX = new QValueAxis();
    axisX->setRange(0, MAX_DATA_POINTS);
    axisX->setLabelFormat("%d");
    axisX->setTitleText(tr("时间 (秒)"));

    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d");
    axisY->setTitleText(tr("温度 (°C)"));

    cpuTempChart->addAxis(axisX, Qt::AlignBottom);
    cpuTempChart->addAxis(axisY, Qt::AlignLeft);
    cpuTempSeries->attachAxis(axisX);
    cpuTempSeries->attachAxis(axisY);

    cpuTempChartView = new QChartView(cpuTempChart);
    cpuTempChartView->setRenderHint(QPainter::Antialiasing);

    // GPU temperature chart
    gpuTempChart = new QChart();
    gpuTempChart->setTitle(tr("GPU温度历史"));
    gpuTempSeries = new QLineSeries();
    gpuTempChart->addSeries(gpuTempSeries);

    QValueAxis* axisX2 = new QValueAxis();
    axisX2->setRange(0, MAX_DATA_POINTS);
    axisX2->setLabelFormat("%d");
    axisX2->setTitleText(tr("时间 (秒)"));

    QValueAxis* axisY2 = new QValueAxis();
    axisY2->setRange(0, 100);
    axisY2->setLabelFormat("%d");
    axisY2->setTitleText(tr("温度 (°C)"));

    gpuTempChart->addAxis(axisX2, Qt::AlignBottom);
    gpuTempChart->addAxis(axisY2, Qt::AlignLeft);
    gpuTempSeries->attachAxis(axisX2);
    gpuTempSeries->attachAxis(axisY2);

    gpuTempChartView = new QChartView(gpuTempChart);
    gpuTempChartView->setRenderHint(QPainter::Antialiasing);

    // Create horizontal splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(cpuTempChartView);
    splitter->addWidget(gpuTempChartView);

    layout->addWidget(splitter);
}

void QtWidgetsTCmonitor::createDiskSection()
{
    diskGroupBox = new QGroupBox(tr("磁盘信息"), this);
    QVBoxLayout* layout = new QVBoxLayout(diskGroupBox);

    // Disk info container
    QWidget* diskContainer = new QWidget();
    layout->addWidget(diskContainer);
}

void QtWidgetsTCmonitor::createNetworkSection()
{
    networkGroupBox = new QGroupBox(tr("网络适配器"), this);
    QGridLayout* layout = new QGridLayout(networkGroupBox);

    int row = 0;
    layout->addWidget(new QLabel(tr("选择适配器:"), this), row, 0);
    networkSelector = new QComboBox(this);
    connect(networkSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QtWidgetsTCmonitor::onNetworkSelectionChanged);
    layout->addWidget(networkSelector, row++, 1);

    layout->addWidget(new QLabel(tr("名称:"), this), row, 0);
    networkNameLabel = new QLabel(this);
    layout->addWidget(networkNameLabel, row++, 1);

    layout->addWidget(new QLabel(tr("MAC地址:"), this), row, 0);
    networkMacLabel = new QLabel(this);
    layout->addWidget(networkMacLabel, row++, 1);

    layout->addWidget(new QLabel(tr("速度:"), this), row, 0);
    networkSpeedLabel = new QLabel(this);
    layout->addWidget(networkSpeedLabel, row++, 1);
}

void QtWidgetsTCmonitor::updateTemperatureData(const std::vector<std::pair<std::string, float>>& temperatures)
{
    float cpuTemp = 0;
    float gpuTemp = 0;
    bool cpuFound = false;
    bool gpuFound = false;

    // Iterate through temperature data
    for (const auto& temp : temperatures) {
        if (temp.first == "CPU Package" || temp.first == "CPU Temperature" || temp.first == "CPU Average Core") {
            cpuTemp = temp.second;
            cpuFound = true;
            infoLabels["cpuTemp"]->setText(formatTemperature(cpuTemp));
        }
        else if (temp.first.find("GPU Core") != std::string::npos) {
            gpuTemp = temp.second;
            gpuFound = true;
            infoLabels["gpuTemp"]->setText(formatTemperature(gpuTemp));
        }
    }

    // If no data found, update to no data state
    if (!cpuFound) {
        infoLabels["cpuTemp"]->setText(tr("无数据"));
    }
    if (!gpuFound) {
        infoLabels["gpuTemp"]->setText(tr("无数据"));
    }

    // Update temperature history
    if (cpuFound) {
        cpuTempHistory.push(cpuTemp);
        if (cpuTempHistory.size() > MAX_DATA_POINTS) {
            cpuTempHistory.pop();
        }
    }

    if (gpuFound) {
        gpuTempHistory.push(gpuTemp);
        if (gpuTempHistory.size() > MAX_DATA_POINTS) {
            gpuTempHistory.pop();
        }
    }

    // Save current system info temperature data
    std::vector<std::pair<std::string, double>> tempDoubles;
    for (const auto& temp : temperatures) {
        tempDoubles.push_back({temp.first, static_cast<double>(temp.second)});
    }
    currentSysInfo.temperatures = tempDoubles;
}

void QtWidgetsTCmonitor::updateSystemInfo(const SystemInfo& sysInfo)
{
    currentSysInfo = sysInfo;

    // Update CPU info
    infoLabels["cpuName"]->setText(QString::fromStdString(sysInfo.cpuName));
    infoLabels["physicalCores"]->setText(QString::number(sysInfo.physicalCores));
    infoLabels["logicalCores"]->setText(QString::number(sysInfo.logicalCores));
    infoLabels["performanceCores"]->setText(QString::number(sysInfo.performanceCores));
    infoLabels["efficiencyCores"]->setText(QString::number(sysInfo.efficiencyCores));
    infoLabels["cpuUsage"]->setText(formatPercentage(sysInfo.cpuUsage));
    infoLabels["hyperThreading"]->setText(sysInfo.hyperThreading ? tr("已启用") : tr("未启用"));
    infoLabels["virtualization"]->setText(sysInfo.virtualization ? tr("已启用") : tr("未启用"));

    // Update memory info
    infoLabels["totalMemory"]->setText(formatSize(sysInfo.totalMemory));
    infoLabels["usedMemory"]->setText(formatSize(sysInfo.usedMemory));
    infoLabels["availableMemory"]->setText(formatSize(sysInfo.availableMemory));
    infoLabels["memoryFrequency"]->setText(QString("%1 MHz").arg(sysInfo.memoryFrequency)); // 新增

    double memoryUsagePercent = static_cast<double>(sysInfo.usedMemory) / sysInfo.totalMemory * 100.0;
    infoLabels["memoryUsage"]->setText(formatPercentage(memoryUsagePercent));

    // Update GPU info
    if (!sysInfo.gpus.empty()) {
        // 检查是否需要更新 GPU 选择器
        static int lastGpuCount = 0;
        if (sysInfo.gpus.size() != lastGpuCount) {
            updateGpuSelector(sysInfo.gpus.size());
            lastGpuCount = sysInfo.gpus.size();
        }

        // 确保索引在有效范围内
        if (currentGpuIndex >= 0 && currentGpuIndex < sysInfo.gpus.size()) {
            infoLabels["gpuName"]->setText(QString::fromStdString(sysInfo.gpus[currentGpuIndex].name));
            infoLabels["gpuVram"]->setText(formatSize(sysInfo.gpus[currentGpuIndex].vram));
            infoLabels["gpuCoreFreq"]->setText(formatFrequency(sysInfo.gpus[currentGpuIndex].coreClock));
        }
    }
    else {
        infoLabels["gpuName"]->setText(tr("无数据"));
        infoLabels["gpuVram"]->setText(tr("无数据"));
        infoLabels["gpuCoreFreq"]->setText(tr("无数据"));
    }

    // Convert temperature data to float for updateTemperatureData
    std::vector<std::pair<std::string, float>> tempFloats;
    for (const auto& temp : sysInfo.temperatures) {
        tempFloats.push_back({temp.first, static_cast<float>(temp.second)});
    }
    
    // Update temperature data
    updateTemperatureData(tempFloats);

    // Update disk info
    // Need to clear existing disk info layout first
    QLayout* currentLayout = diskGroupBox->layout();
    QWidget* diskContainer = nullptr;

    if (currentLayout) {
        // Get disk container from current layout
        for (int i = 0; i < currentLayout->count(); ++i) {
            QWidget* widget = currentLayout->itemAt(i)->widget();
            if (widget) {
                diskContainer = widget;
                break;
            }
        }
    }

    // If container not found, create a new one
    if (!diskContainer) {
        diskContainer = new QWidget(diskGroupBox);
        static_cast<QVBoxLayout*>(currentLayout)->addWidget(diskContainer);
    }

    // Delete existing layout
    if (diskContainer->layout()) {
        QLayoutItem* child;
        while ((child = diskContainer->layout()->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
        delete diskContainer->layout();
    }

    QVBoxLayout* diskLayout = new QVBoxLayout(diskContainer);

    for (const auto& disk : sysInfo.disks) {
        QString diskLabel = QString("%1: %2").arg(QChar(disk.letter)).arg(tr("驱动器"));
        if (!disk.label.empty()) {
            diskLabel += QString(" (%1)").arg(QString::fromStdString(disk.label));
        }

        QGroupBox* diskBox = new QGroupBox(diskLabel);
        QGridLayout* diskInfoLayout = new QGridLayout(diskBox);

        int row = 0;
        if (!disk.fileSystem.empty()) {
            diskInfoLayout->addWidget(new QLabel(tr("文件系统:"), diskBox), row, 0);
            diskInfoLayout->addWidget(new QLabel(QString::fromStdString(disk.fileSystem), diskBox), row++, 1);
        }

        diskInfoLayout->addWidget(new QLabel(tr("总容量:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(disk.totalSize), diskBox), row++, 1);

        diskInfoLayout->addWidget(new QLabel(tr("已用空间:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(disk.usedSpace), diskBox), row++, 1);

        uint64_t freeSpace = disk.totalSize - disk.usedSpace;
        diskInfoLayout->addWidget(new QLabel(tr("可用空间:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(freeSpace), diskBox), row++, 1);

        double usagePercent = static_cast<double>(disk.usedSpace) / disk.totalSize * 100.0;
        diskInfoLayout->addWidget(new QLabel(tr("使用率:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatPercentage(usagePercent), diskBox), row++, 1);

        diskLayout->addWidget(diskBox);
    }

    diskLayout->addStretch();
}

void QtWidgetsTCmonitor::updateCharts()
{
    // Update CPU temperature chart
    cpuTempSeries->clear();
    int index = 0;
    std::queue<float> tempCpuTemp = cpuTempHistory; // Make a copy to preserve the original
    while (!tempCpuTemp.empty()) {
        cpuTempSeries->append(index, tempCpuTemp.front());
        tempCpuTemp.pop();
        index++;
    }

    // Update GPU temperature chart
    gpuTempSeries->clear();
    index = 0;
    std::queue<float> tempGpuTemp = gpuTempHistory; // Make a copy to preserve the original
    while (!tempGpuTemp.empty()) {
        gpuTempSeries->append(index, tempGpuTemp.front());
        tempGpuTemp.pop();
        index++;
    }
}

void QtWidgetsTCmonitor::on_pushButton_clicked()
{
    QMessageBox::information(this, tr("系统监控"), tr("正在监控系统硬件信息"));
}

QString QtWidgetsTCmonitor::formatSize(uint64_t bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    QString unit;
    double size;

    if (bytes >= TB) {
        size = bytes / TB;
        unit = "TB";
    } else if (bytes >= GB) {
        size = bytes / GB;
        unit = "GB";
    } else if (bytes >= MB) {
        size = bytes / MB;
        unit = "MB";
    } else if (bytes >= KB) {
        size = bytes / KB;
        unit = "KB";
    } else {
        size = bytes;
        unit = "B";
    }

    return QString("%1 %2").arg(size, 0, 'f', 2).arg(unit);
}

QString QtWidgetsTCmonitor::formatPercentage(double value)
{
    return QString("%1%").arg(value, 0, 'f', 1);
}

QString QtWidgetsTCmonitor::formatTemperature(double value)
{
    return QString("%1°C").arg(value, 0, 'f', 1);
}

QString QtWidgetsTCmonitor::formatFrequency(double value)
{
    if (value >= 1000) {
        return QString("%1 GHz").arg(value / 1000, 0, 'f', 2);
    } else {
        return QString("%1 MHz").arg(value, 0, 'f', 0);
    }
}

void QtWidgetsTCmonitor::updateGpuSelector(int gpuCount)
{
    if (!gpuSelector || gpuCount <= 0) return;

    // 暂时断开信号连接，防止触发不必要的更新
    gpuSelector->blockSignals(true);

    // 保存当前选择的项目名称，用于尝试恢复选择
    QString currentSelection;
    if (gpuSelector->currentIndex() >= 0) {
        currentSelection = gpuSelector->currentText();
    }

    // 清除现有项目
    gpuSelector->clear();
    gpuIndices.clear();

    // 从共享内存缓冲区获取 GPU 数据
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) return;

    // 构建 GPU 索引数组，按计算能力排序
    struct GpuInfo {
        int index;
        std::wstring name;
        int computeCapability;  // 计算能力 = major*100 + minor
    };

    std::vector<GpuInfo> gpuInfos;
    for (int i = 0; i < gpuCount; ++i) {
        GpuInfo info;
        info.index = i;
        info.name = std::wstring(pBuffer->gpus[i].name);

        // 提取计算能力，这里假设该信息位于 name 字符串中
        // 如 "NVIDIA GeForce RTX 4060 (Compute 8.9)"
        // 简单实现，实际应从 GpuInfo 类获取计算能力
        int computeCapability = 0;

        // 对于 NVIDIA 显卡，我们尝试从名称中提取计算能力
        // 优先级：NVIDIA > AMD > Intel
        if (info.name.find(L"NVIDIA") != std::wstring::npos) {
            computeCapability = 1000;  // 基础分高
        }
        else if (info.name.find(L"AMD") != std::wstring::npos ||
            info.name.find(L"Radeon") != std::wstring::npos) {
            computeCapability = 500;
        }
        else if (info.name.find(L"Intel") != std::wstring::npos) {
            computeCapability = 100;
        }

        // 额外考虑： RTX > GTX > 其他
        if (info.name.find(L"RTX") != std::wstring::npos) {
            computeCapability += 300;
        }
        else if (info.name.find(L"GTX") != std::wstring::npos) {
            computeCapability += 200;
        }

        info.computeCapability = computeCapability;
        gpuInfos.push_back(info);
    }

    // 按计算能力排序，最强大的显卡排在最前面
    std::sort(gpuInfos.begin(), gpuInfos.end(),
        [](const GpuInfo& a, const GpuInfo& b) {
            return a.computeCapability > b.computeCapability;
        });

    // 将排序后的 GPU 添加到选择器中
    for (const auto& info : gpuInfos) {
        QString displayName = WinUtils::WstringToQString(info.name);
        gpuSelector->addItem(displayName);
        gpuIndices.push_back(info.index);  // 保存索引映射
    }

    // 尝试恢复之前的选择，如果不存在则选择第一个（最强大的）
    int newIndex = 0;  // 默认选择第一个
    if (!currentSelection.isEmpty()) {
        int foundIndex = gpuSelector->findText(currentSelection);
        if (foundIndex >= 0) {
            newIndex = foundIndex;
        }
    }

    gpuSelector->setCurrentIndex(newIndex);
    currentGpuIndex = gpuIndices[newIndex];

    // 恢复信号连接
    gpuSelector->blockSignals(false);
}

// Ensure data is read from shared memory and updates the UI
void QtWidgetsTCmonitor::updateFromSharedMemory() {
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) return;

    try {
        // Enter critical section to safely access shared memory
        EnterCriticalSection(&pBuffer->lock);

        // 防御性判空检查
        if (infoLabels.contains("cpuName") && infoLabels["cpuName"]) {
            if (isWCharArrayNullTerminated(pBuffer->cpuName, 128)) {
                infoLabels["cpuName"]->setText(WinUtils::WstringToQString(std::wstring(pBuffer->cpuName)));
            } else {
                infoLabels["cpuName"]->setText(tr("无数据"));
            }
        }

        if (infoLabels.contains("physicalCores") && infoLabels["physicalCores"]) {
            infoLabels["physicalCores"]->setText(QString::number(pBuffer->physicalCores));
        }
        if (infoLabels.contains("logicalCores") && infoLabels["logicalCores"]) {
            infoLabels["logicalCores"]->setText(QString::number(pBuffer->logicalCores));
        }
        if (infoLabels.contains("cpuUsage") && infoLabels["cpuUsage"]) {
            infoLabels["cpuUsage"]->setText(formatPercentage(pBuffer->cpuUsage));
        }
        if (infoLabels.contains("performanceCores") && infoLabels["performanceCores"]) {
            infoLabels["performanceCores"]->setText(QString::number(pBuffer->performanceCores));
        }
        if (infoLabels.contains("efficiencyCores") && infoLabels["efficiencyCores"]) {
            infoLabels["efficiencyCores"]->setText(QString::number(pBuffer->efficiencyCores));
        }
        if (infoLabels.contains("hyperThreading") && infoLabels["hyperThreading"]) {
            infoLabels["hyperThreading"]->setText(pBuffer->hyperThreading ? tr("已启用") : tr("未启用"));
        }
        if (infoLabels.contains("virtualization") && infoLabels["virtualization"]) {
            infoLabels["virtualization"]->setText(pBuffer->virtualization ? tr("已启用") : tr("未启用"));
        }

        // Update memory information
        infoLabels["totalMemory"]->setText(formatSize(pBuffer->totalMemory));
        infoLabels["usedMemory"]->setText(formatSize(pBuffer->usedMemory));
        infoLabels["availableMemory"]->setText(formatSize(pBuffer->availableMemory));
        infoLabels["memoryFrequency"]->setText(QString("%1 MHz").arg(pBuffer->memoryFrequency)); // 新增
        
        double memoryUsagePercent = static_cast<double>(pBuffer->usedMemory) / pBuffer->totalMemory * 100.0;
        infoLabels["memoryUsage"]->setText(formatPercentage(memoryUsagePercent));

        // 在 updateFromSharedMemory 方法中
        // GPU 信息更新部分
        if (pBuffer->gpuCount > 0) {
            // 检查是否需要更新 GPU 选择器
            static int lastGpuCount = 0;
            if (pBuffer->gpuCount != lastGpuCount) {
                updateGpuSelector(pBuffer->gpuCount);
                lastGpuCount = pBuffer->gpuCount;
            }

            // 确保索引在有效范围内
            if (currentGpuIndex >= 0 && currentGpuIndex < pBuffer->gpuCount) {
                // 名称
                if (infoLabels.contains("gpuName") && infoLabels["gpuName"] &&
                    isWCharArrayNullTerminated(pBuffer->gpus[currentGpuIndex].name, 128)) {
                    infoLabels["gpuName"]->setText(WinUtils::WstringToQString(std::wstring(pBuffer->gpus[currentGpuIndex].name)));
                }

                // 专用显存 (VRAM)
                if (infoLabels.contains("gpuVram") && infoLabels["gpuVram"]) {
                    if (pBuffer->gpus[currentGpuIndex].vram > 0) {
                        infoLabels["gpuVram"]->setText(formatSize(pBuffer->gpus[currentGpuIndex].vram));
                    }
                    else {
                        infoLabels["gpuVram"]->setText(tr("未知"));
                    }
                }

                // 共享内存
                if (infoLabels.contains("gpuSharedMem") && infoLabels["gpuSharedMem"]) {
                    if (pBuffer->gpus[currentGpuIndex].sharedMemory > 0) {
                        infoLabels["gpuSharedMem"]->setText(formatSize(pBuffer->gpus[currentGpuIndex].sharedMemory));
                    }
                    else {
                        infoLabels["gpuSharedMem"]->setText(tr("无"));
                    }
                }

                // 核心频率
                if (infoLabels.contains("gpuCoreFreq") && infoLabels["gpuCoreFreq"]) {
                    infoLabels["gpuCoreFreq"]->setText(formatFrequency(pBuffer->gpus[currentGpuIndex].coreClock));
                }
            }

            if (infoLabels.contains("gpuPower") && infoLabels["gpuPower"]) {
                float totalGpuPower = 0.0f;
                int validGpuCount = 0;
                for (int i = 0; i < pBuffer->gpuCount; ++i) {
                    float power = pBuffer->gpus[i].power;
                    if (!std::isnan(power) && power > 0) {
                        totalGpuPower += power;
                        ++validGpuCount;
                    }
                }
                if (validGpuCount == 0) {
                    infoLabels["gpuPower"]->setText(tr("未支持"));
                }
                else if (validGpuCount == 1) {
                    infoLabels["gpuPower"]->setText(QString::number(totalGpuPower, 'f', 1) + " W");
                }
                else {
                    infoLabels["gpuPower"]->setText(
                        QString("%1 W (%2)").arg(totalGpuPower, 0, 'f', 1).arg(tr("多卡合计，可能不准确"))
                    );
                }
            }
        }
        else {
            // GPU信息不可用时的处理
            if (infoLabels.contains("gpuName") && infoLabels["gpuName"]) {
                infoLabels["gpuName"]->setText(tr("无数据"));
            }
            if (infoLabels.contains("gpuBrand") && infoLabels["gpuBrand"]) {
                infoLabels["gpuBrand"]->setText(tr("无数据"));
            }
            if (infoLabels.contains("gpuVram") && infoLabels["gpuVram"]) {
                infoLabels["gpuVram"]->setText(tr("无数据"));
            }
            if (infoLabels.contains("gpuSharedMem") && infoLabels["gpuSharedMem"]) {
                infoLabels["gpuSharedMem"]->setText(tr("无数据"));
            }
            if (infoLabels.contains("gpuCoreFreq") && infoLabels["gpuCoreFreq"]) {
                infoLabels["gpuCoreFreq"]->setText(tr("无数据"));
            }
            if (infoLabels.contains("gpuPower") && infoLabels["gpuPower"]) {
                infoLabels["gpuPower"]->setText(tr("未支持"));
            }
        }


        // Update temperature data and charts
        float cpuTemp = 0;
        float gpuTemp = 0;
        bool cpuFound = false;
        bool gpuFound = false;

        // Parse temperature sensors to find CPU and GPU temperatures
        for (int i = 0; i < pBuffer->tempCount; i++) {
            // 直接用wchar_t[32]，并调试输出
            std::wstring ws(pBuffer->temperatures[i].sensorName, 32);
            // 去除多余尾部0
            ws = ws.c_str();
            qDebug() << "TempSensorName:" << WinUtils::WstringToQString(ws)
                     << "Value:" << pBuffer->temperatures[i].temperature;

            // 判断名称是否为"CPU温度"或"GPU温度"
            if (ws == L"CPU温度") {
                float value = pBuffer->temperatures[i].temperature;
                if (std::isnan(value)) {
                    infoLabels["cpuTemp"]->setText(tr("无数据"));
                } else {
                    infoLabels["cpuTemp"]->setText(formatTemperature(value));
                }
                cpuTemp = value;
                cpuFound = true;
            } else if (ws == L"GPU温度") {
                float value = pBuffer->temperatures[i].temperature;
                if (std::isnan(value)) {
                    infoLabels["gpuTemp"]->setText(tr("无数据"));
                } else {
                    infoLabels["gpuTemp"]->setText(formatTemperature(value));
                }
                gpuTemp = value;
                gpuFound = true;
            }
        }

        // If no specific temperature data found, set to no data
        if (!cpuFound && infoLabels.contains("cpuTemp") && infoLabels["cpuTemp"]) {
            infoLabels["cpuTemp"]->setText(tr("无数据"));
        }
        if (!gpuFound && infoLabels.contains("gpuTemp") && infoLabels["gpuTemp"]) {
            infoLabels["gpuTemp"]->setText(tr("无数据"));
        }

        // Update temperature history for charts
        if (cpuFound && !std::isnan(cpuTemp)) {
            cpuTempHistory.push(cpuTemp);
            if (cpuTempHistory.size() > MAX_DATA_POINTS) {
                cpuTempHistory.pop();
            }
        }

        if (gpuFound && !std::isnan(gpuTemp)) {
            gpuTempHistory.push(gpuTemp);
            if (gpuTempHistory.size() > MAX_DATA_POINTS) {
                gpuTempHistory.pop();
            }
        }

        // CPU功率显示
        if (infoLabels.contains("cpuPower") && infoLabels["cpuPower"]) {
            float cpuPower = pBuffer->cpuPower;
            if (std::isnan(cpuPower) || cpuPower <= 0) {
                infoLabels["cpuPower"]->setText(tr("-- W"));
            } else {
                infoLabels["cpuPower"]->setText(QString::number(cpuPower, 'f', 1) + " W");
            }
        }

        // GPU功率显示
        if (infoLabels.contains("gpuPower") && infoLabels["gpuPower"]) {
            float gpuPower = pBuffer->gpuPower;
            if (std::isnan(gpuPower) || gpuPower <= 0) {
                infoLabels["gpuPower"]->setText(tr("未支持"));
            } else {
                infoLabels["gpuPower"]->setText(QString::number(gpuPower, 'f', 1) + " W");
            }
        }

        // 整机功率
        if (infoLabels.contains("totalPower") && infoLabels["totalPower"]) {
            infoLabels["totalPower"]->setText(QString("%1 W").arg(pBuffer->totalPower, 0, 'f', 2));
        }

        if (infoLabels.contains("motherboardName") && infoLabels["motherboardName"]) {
            if (isWCharArrayNullTerminated(pBuffer->motherboardName, 128))
                infoLabels["motherboardName"]->setText(WinUtils::WstringToQString(std::wstring(pBuffer->motherboardName)));
            else
                infoLabels["motherboardName"]->setText(tr("无数据"));
        }
        if (infoLabels.contains("deviceName") && infoLabels["deviceName"]) {
            if (isWCharArrayNullTerminated(pBuffer->deviceName, 128))
                infoLabels["deviceName"]->setText(WinUtils::WstringToQString(std::wstring(pBuffer->deviceName)));
            else
                infoLabels["deviceName"]->setText(tr("无数据"));
        }
        if (infoLabels.contains("osVersion") && infoLabels["osVersion"]) {
            if (isWCharArrayNullTerminated(pBuffer->osDetailedVersion, 256))
                infoLabels["osVersion"]->setText(WinUtils::WstringToQString(std::wstring(pBuffer->osDetailedVersion)));
            else
                infoLabels["osVersion"]->setText(tr("无数据"));
        }

        // 网络适配器信息
        if (pBuffer->adapterCount > 0) {
            static int lastAdapterCount = 0;
            if (pBuffer->adapterCount != lastAdapterCount) {
                updateNetworkSelector(pBuffer->adapterCount, pBuffer);
                lastAdapterCount = pBuffer->adapterCount;
            }
            if (currentNetworkIndex >= 0 && currentNetworkIndex < pBuffer->adapterCount) {
                const auto& adapter = pBuffer->adapters[currentNetworkIndex];
                networkNameLabel->setText(isWCharArrayNullTerminated(adapter.name, 128)
                    ? WinUtils::WstringToQString(std::wstring(adapter.name)) : tr("未知"));
                networkMacLabel->setText(isWCharArrayNullTerminated(adapter.mac, 32)
                    ? WinUtils::WstringToQString(std::wstring(adapter.mac)) : tr("未知"));
                networkSpeedLabel->setText(QString("%1 Mbps").arg(adapter.speed / 1000000.0, 0, 'f', 1));
            }
        }
        else {
            networkNameLabel->setText(tr("无数据"));
            networkMacLabel->setText(tr("无数据"));
            networkSpeedLabel->setText(tr("无数据"));
        }


        // Update disk information
        updateDiskInfo(pBuffer);
        
        // Update charts
        updateCharts();

        // Leave critical section
        LeaveCriticalSection(&pBuffer->lock);
    }
    catch (const std::exception& e) {
        // Safely release critical section if an exception occurs
        LeaveCriticalSection(&pBuffer->lock);
        QMessageBox::critical(this, tr("错误"), 
            tr("读取共享内存时出错: %1").arg(e.what()));
    }
    catch (...) {
        // Safely release critical section if an unknown exception occurs
        LeaveCriticalSection(&pBuffer->lock);
        QMessageBox::critical(this, tr("错误"), tr("读取共享内存时出现未知错误"));
    }
}

// New helper method to update disk information
void QtWidgetsTCmonitor::updateDiskInfo(SharedMemoryBlock* pBuffer) {
    if (!pBuffer || pBuffer->diskCount <= 0) return;

    // Need to clear existing disk info layout first
    QLayout* currentLayout = diskGroupBox->layout();
    QWidget* diskContainer = nullptr;

    if (currentLayout) {
        // Get disk container from current layout
        for (int i = 0; i < currentLayout->count(); ++i) {
            QWidget* widget = currentLayout->itemAt(i)->widget();
            if (widget) {
                diskContainer = widget;
                break;
            }
        }
    }

    // If container not found, create a new one
    if (!diskContainer) {
        diskContainer = new QWidget(diskGroupBox);
        static_cast<QVBoxLayout*>(currentLayout)->addWidget(diskContainer);
    }

    // Delete existing layout
    if (diskContainer->layout()) {
        QLayoutItem* child;
        while ((child = diskContainer->layout()->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
        delete diskContainer->layout();
    }

    // Create new layout
    QVBoxLayout* diskLayout = new QVBoxLayout(diskContainer);

    // Add disk info for each disk in shared memory
    for (int i = 0; i < pBuffer->diskCount; i++) {
        auto& disk = pBuffer->disks[i];
        QString diskLabel = QString("%1: %2").arg(QChar(disk.letter)).arg(tr("驱动器"));
        
        QString label = isWCharArrayNullTerminated(disk.label, 128)
            ? WinUtils::WstringToQString(std::wstring(disk.label))
            : QString();
        if (!label.isEmpty()) {
            diskLabel += QString(" (%1)").arg(label);
        }

        QGroupBox* diskBox = new QGroupBox(diskLabel);
        QGridLayout* diskInfoLayout = new QGridLayout(diskBox);

        int row = 0;
        QString fileSystem = isWCharArrayNullTerminated(disk.fileSystem, 32)
            ? WinUtils::WstringToQString(std::wstring(disk.fileSystem))
            : QString();
        if (!fileSystem.isEmpty()) {
            diskInfoLayout->addWidget(new QLabel(tr("文件系统:"), diskBox), row, 0);
            diskInfoLayout->addWidget(new QLabel(fileSystem, diskBox), row++, 1);
        }

        diskInfoLayout->addWidget(new QLabel(tr("总容量:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(disk.totalSize), diskBox), row++, 1);

        diskInfoLayout->addWidget(new QLabel(tr("已用空间:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(disk.usedSpace), diskBox), row++, 1);

        uint64_t freeSpace = disk.freeSpace;
        diskInfoLayout->addWidget(new QLabel(tr("可用空间:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatSize(freeSpace), diskBox), row++, 1);

        double usagePercent = static_cast<double>(disk.usedSpace) / disk.totalSize * 100.0;
        diskInfoLayout->addWidget(new QLabel(tr("使用率:"), diskBox), row, 0);
        diskInfoLayout->addWidget(new QLabel(formatPercentage(usagePercent), diskBox), row++, 1);

        // Add progress bar for visual representation
        QProgressBar* usageBar = new QProgressBar(diskBox);
        usageBar->setValue(static_cast<int>(usagePercent));
        usageBar->setTextVisible(false);
        if (usagePercent > 90)
            usageBar->setStyleSheet("QProgressBar::chunk { background-color: #FF4136; }"); // Red for high usage
        else if (usagePercent > 70)
            usageBar->setStyleSheet("QProgressBar::chunk { background-color: #FF851B; }"); // Orange for medium-high usage
        diskInfoLayout->addWidget(usageBar, row++, 0, 1, 2);

        diskLayout->addWidget(diskBox);
    }

    diskLayout->addStretch();
}

void QtWidgetsTCmonitor::UpdateDiskInfoUI() {
    const auto& disks = SharedMemoryManager::GetSystemData().disks;
    ui->diskTable->setRowCount(static_cast<int>(disks.size()));
    for (int i = 0; i < disks.size(); ++i) {
        const auto& d = disks[i];
        ui->diskTable->setItem(i, 0, new QTableWidgetItem(QString(d.letter)));
        ui->diskTable->setItem(i, 1, new QTableWidgetItem(WinUtils::Utf8StringToQString(d.label)));
        ui->diskTable->setItem(i, 2, new QTableWidgetItem(WinUtils::Utf8StringToQString(d.fileSystem)));
        ui->diskTable->setItem(i, 3, new QTableWidgetItem(QString("%1 GB").arg(d.totalSize / (1024 * 1024 * 1024.0), 0, 'f', 2)));
        ui->diskTable->setItem(i, 4, new QTableWidgetItem(QString("%1 GB").arg(d.usedSpace / (1024 * 1024 * 1024.0), 0, 'f', 2)));
        ui->diskTable->setItem(i, 5, new QTableWidgetItem(QString("%1 GB").arg(d.freeSpace / (1024 * 1024 * 1024.0), 0, 'f', 2)));
    }
}

void QtWidgetsTCmonitor::updateNetworkSelector(int adapterCount, SharedMemoryBlock* pBuffer)
{
    if (!networkSelector || adapterCount <= 0) return;

    networkSelector->blockSignals(true);

    QString currentSelection;
    if (networkSelector->currentIndex() >= 0)
        currentSelection = networkSelector->currentText();

    networkSelector->clear();
    networkIndices.clear();

    for (int i = 0; i < adapterCount; ++i) {
        QString name = isWCharArrayNullTerminated(pBuffer->adapters[i].name, 128)
            ? WinUtils::WstringToQString(std::wstring(pBuffer->adapters[i].name))
            : tr("未知");
        networkSelector->addItem(name);
        networkIndices.push_back(i);
    }

    int newIndex = 0;
    if (!currentSelection.isEmpty()) {
        int foundIndex = networkSelector->findText(currentSelection);
        if (foundIndex >= 0) newIndex = foundIndex;
    }
    networkSelector->setCurrentIndex(newIndex);
    currentNetworkIndex = networkIndices[newIndex];

    networkSelector->blockSignals(false);
}

void QtWidgetsTCmonitor::onGpuSelectionChanged(int index)
{
    if (index < 0 || index >= gpuIndices.size()) return;
    currentGpuIndex = gpuIndices[index];
    updateFromSharedMemory();
}

void QtWidgetsTCmonitor::onNetworkSelectionChanged(int index)
{
    if (index < 0 || index >= networkIndices.size()) return;
    currentNetworkIndex = networkIndices[index];
    updateFromSharedMemory();
}

// 在合适的地方调用 UpdateDiskInfoUI()，如定时刷新或窗口初始化时

