#include "stdafx.h"
#include "QtWidgetsTCmonitor.h"
#include "ui_QtWidgetsTCmonitor.h"  // Auto-generated UI file
#include <windows.h>
#include "../src/core/DataStruct/SharedMemoryManager.h"
#include "../src/core/Utils/WinUtils.h"  // Include WinUtils for string conversion
#include "../src/core/disk/DiskInfo.h" // Include DiskInfo for disk-related operations

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

    // 只保留驱动版本
    layout->addWidget(new QLabel(tr("GPU驱动版本:"), this), row, 0);
    gpuDriverVersionLabel = new QLabel(this);
    layout->addWidget(gpuDriverVersionLabel, row++, 1);

    // 初始化GPU选择器（只读一次）
    updateGpuSelector();
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

    // Disk info tree widget
    ui->treeWidgetDiskInfo = new QTreeWidget(this);
    ui->treeWidgetDiskInfo->setColumnCount(7);
    QStringList headers;
    headers << tr("名称") << tr("类型") << tr("协议") << tr("总容量") << tr("已用空间") << tr("使用率") << tr("SMART状态");
    ui->treeWidgetDiskInfo->setHeaderLabels(headers);
    layout->addWidget(ui->treeWidgetDiskInfo);
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

    layout->addWidget(new QLabel(tr("连接状态:"), this), row, 0);
    networkStatusLabel = new QLabel(this);
    layout->addWidget(networkStatusLabel, row++, 1);

    layout->addWidget(new QLabel(tr("IP地址:"), this), row, 0);
    networkIpLabel = new QLabel(this);
    layout->addWidget(networkIpLabel, row++, 1);

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

    for (const auto& temp : temperatures) {
        if (temp.first == "CPU Package" || temp.first == "CPU Temperature" || temp.first == "CPU Average Core" || temp.first == "CPU温度") {
            cpuTemp = temp.second;
            cpuFound = true;
            infoLabels["cpuTemp"]->setText(formatTemperature(cpuTemp));
        }
        // 支持更多GPU温度名称
        else if (temp.first.find("GPU Core") != std::string::npos || temp.first.find("GPU温度") != std::string::npos || temp.first.find("GPU Temperature") != std::string::npos) {
            gpuTemp = temp.second;
            gpuFound = true;
            infoLabels["gpuTemp"]->setText(formatTemperature(gpuTemp));
        }
    }

    if (!cpuFound) {
        infoLabels["cpuTemp"]->setText(tr("无数据"));
    }
    if (!gpuFound) {
        infoLabels["gpuTemp"]->setText(tr("无数据"));
    }

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
        // 不再每次都调用updateGpuSelector
        // 只需确保currentGpuIndex有效
        if (currentGpuIndex < 0 || currentGpuIndex >= sysInfo.gpus.size()) {
            currentGpuIndex = 0;
        }

        infoLabels["gpuName"]->setText(QString::fromStdString(sysInfo.gpus[currentGpuIndex].name));
        infoLabels["gpuVram"]->setText(formatSize(sysInfo.gpus[currentGpuIndex].vram));
        infoLabels["gpuSharedMem"]->setText(formatSize(sysInfo.gpus[currentGpuIndex].sharedMemory));
        infoLabels["gpuCoreFreq"]->setText(formatFrequency(sysInfo.gpus[currentGpuIndex].coreClock));

        // 只保留驱动版本
        gpuDriverVersionLabel->setText(QString::fromStdWString(sysInfo.gpus[currentGpuIndex].driverVersion));
    }
    else {
        infoLabels["gpuName"]->setText(tr("无数据"));
        infoLabels["gpuVram"]->setText(tr("无数据"));
        infoLabels["gpuSharedMem"]->setText(tr("无数据"));
        infoLabels["gpuCoreFreq"]->setText(tr("无数据"));

        gpuDriverVersionLabel->setText(tr("无数据"));
    }

    // Convert temperature data to float for updateTemperatureData
    std::vector<std::pair<std::string, float>> tempFloats;
    for (const auto& temp : sysInfo.temperatures) {
        tempFloats.push_back({temp.first, static_cast<float>(temp.second)});
    }
    
    // Update temperature data
    updateTemperatureData(tempFloats);

    // Update disk info
    updateDiskTreeWidget();
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

void QtWidgetsTCmonitor::updateGpuSelector()
{
    if (!gpuSelector) return;

    gpuSelector->blockSignals(true);

    QString currentSelection;
    if (gpuSelector->currentIndex() >= 0) {
        currentSelection = gpuSelector->currentText();
    }

    gpuSelector->clear();
    gpuIndices.clear();

    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) {
        gpuSelector->blockSignals(false);
        return;
    }

    int gpuCount = pBuffer->gpuCount;
    cachedGpuCount = gpuCount; // 始终刷新缓存

    if (gpuCount <= 0) {
        gpuSelector->blockSignals(false);
        return;
    }

    struct GpuInfo {
        int index;
        std::wstring name;
        int computeCapability;
    };

    std::vector<GpuInfo> gpuInfos;
    for (int i = 0; i < gpuCount; ++i) {
        GpuInfo info;
        info.index = i;
        info.name = std::wstring(pBuffer->gpus[i].name);

        int computeCapability = 0;
        if (info.name.find(L"NVIDIA") != std::wstring::npos) {
            computeCapability = 1000;
        }
        else if (info.name.find(L"AMD") != std::wstring::npos ||
                 info.name.find(L"Radeon") != std::wstring::npos) {
            computeCapability = 500;
        }
        else if (info.name.find(L"Intel") != std::wstring::npos) {
            computeCapability = 100;
        }
        if (info.name.find(L"RTX") != std::wstring::npos) {
            computeCapability += 300;
        }
        else if (info.name.find(L"GTX") != std::wstring::npos) {
            computeCapability += 200;
        }
        info.computeCapability = computeCapability;
        gpuInfos.push_back(info);
    }

    std::sort(gpuInfos.begin(), gpuInfos.end(),
        [](const GpuInfo& a, const GpuInfo& b) {
            return a.computeCapability > b.computeCapability;
        });

    for (const auto& info : gpuInfos) {
        QString displayName = WinUtils::WstringToQString(info.name);
        gpuSelector->addItem(displayName);
        gpuIndices.push_back(info.index);
    }

    int newIndex = 0;
    if (!currentSelection.isEmpty()) {
        int foundIndex = gpuSelector->findText(currentSelection);
        if (foundIndex >= 0) {
            newIndex = foundIndex;
        }
    }
    gpuSelector->setCurrentIndex(newIndex);
    currentGpuIndex = gpuIndices[newIndex];

    gpuSelector->blockSignals(false);
}

void QtWidgetsTCmonitor::updateFromSharedMemory() {
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) {
        qDebug() << "[调试] 共享内存未初始化或获取失败";
        return;
    }

    try {
        EnterCriticalSection(&pBuffer->lock);

        // 新增调试日志，确认数据同步
        qDebug() << "[调试] 共享内存物理核心:" << pBuffer->physicalCores
                 << " GPU数量:" << pBuffer->gpuCount
                 << " CPU名称:" << QString::fromStdWString(std::wstring(pBuffer->cpuName));

        SystemInfo sysInfo;
        sysInfo.cpuName = WinUtils::WstringToQString(std::wstring(pBuffer->cpuName)).toStdString();
        sysInfo.physicalCores = pBuffer->physicalCores;
        sysInfo.logicalCores = pBuffer->logicalCores;
        sysInfo.performanceCores = pBuffer->performanceCores;
        sysInfo.efficiencyCores = pBuffer->efficiencyCores;
        sysInfo.cpuUsage = pBuffer->cpuUsage;
        sysInfo.hyperThreading = pBuffer->hyperThreading;
        sysInfo.virtualization = pBuffer->virtualization;
        sysInfo.totalMemory = pBuffer->totalMemory;
        sysInfo.usedMemory = pBuffer->usedMemory;
        sysInfo.availableMemory = pBuffer->availableMemory;
        sysInfo.memoryFrequency = pBuffer->memoryFrequency;

        sysInfo.gpus.clear();
        for (int i = 0; i < pBuffer->gpuCount; ++i) {
            GPUInfo gpu;
            gpu.name = WinUtils::WstringToQString(std::wstring(pBuffer->gpus[i].name)).toStdString();
            gpu.vram = pBuffer->gpus[i].vram;
            gpu.sharedMemory = pBuffer->gpus[i].sharedMemory;
            gpu.coreClock = pBuffer->gpus[i].coreClock;
            gpu.driverVersion = WinUtils::WstringToQString(std::wstring(pBuffer->gpus[i].driverVersion)).toStdWString();
            // 新增字段
            gpu.available = (pBuffer->gpus[i].available != 0);
            gpu.status = WinUtils::WstringToQString(std::wstring(pBuffer->gpus[i].status)).toStdString();
            gpu.temperature = pBuffer->gpus[i].temperature;
            sysInfo.gpus.push_back(gpu);
        }

        sysInfo.temperatures.clear();
        // 修正：使用tempCount而不是temperatureCount
        for (int i = 0; i < pBuffer->tempCount; ++i) {
            sysInfo.temperatures.push_back({
                WinUtils::WstringToQString(std::wstring(pBuffer->temperatures[i].sensorName)).toStdString(),
                pBuffer->temperatures[i].temperature
            });
        }

        LeaveCriticalSection(&pBuffer->lock);

        updateSystemInfo(sysInfo);
        updateDiskTreeWidget();
        updateCharts();

    } catch (...) {
        LeaveCriticalSection(&pBuffer->lock);
        qDebug() << "[调试] updateFromSharedMemory异常";
    }
}

void QtWidgetsTCmonitor::updateDiskTreeWidget()
{
    if (!ui->treeWidgetDiskInfo) return; 
    ui->treeWidgetDiskInfo->clear();

    auto physicalDisks = DiskInfo::GetAllPhysicalDisks();

    for (const auto& pdisk : physicalDisks) {
        QTreeWidgetItem* diskItem = new QTreeWidgetItem(ui->treeWidgetDiskInfo);
        QString displayName = QString::fromStdString(pdisk.model);
        if (displayName.isEmpty()) {
            displayName = QString::fromStdString(pdisk.name);
        }
        diskItem->setText(0, displayName);
        diskItem->setText(1, QString::fromStdString(pdisk.type));
        diskItem->setText(2, QString::fromStdString(pdisk.protocol));
        diskItem->setText(3, QString::fromStdString(DiskInfo::FormatSize(pdisk.totalSize)));
        diskItem->setText(4, "");
        diskItem->setText(5, "");
        diskItem->setText(6, QString::fromStdString(pdisk.smartStatus));

        QPushButton* btn = new QPushButton(tr("SMART详情"), ui->treeWidgetDiskInfo);
        ui->treeWidgetDiskInfo->setItemWidget(diskItem, 6, btn);
        
        connect(btn, &QPushButton::clicked, this, [this, diskIdentifier = pdisk.name]() {
            showSmartDetails(QString::fromStdString(diskIdentifier));
        });
    }
    ui->treeWidgetDiskInfo->expandAll();
    for(int i = 0; i < ui->treeWidgetDiskInfo->columnCount(); ++i) {
        ui->treeWidgetDiskInfo->resizeColumnToContents(i);
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

void QtWidgetsTCmonitor::showSmartDetails(const QString& diskIdentifier)
{
    auto attrs = DiskInfo::GetSmartAttributes(diskIdentifier.toStdString()); 
    QDialog dlg(this);
    dlg.setWindowTitle(tr("SMART详细信息 - %1").arg(diskIdentifier)); 
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QTableWidget* table = new QTableWidget(static_cast<int>(attrs.size()), 6, &dlg);
    table->setHorizontalHeaderLabels({tr("ID"), tr("名称"), tr("当前值"), tr("最差值"), tr("阈值"), tr("原始值")});
    int row = 0;
    for (const auto& a : attrs) {
        table->setItem(row, 0, new QTableWidgetItem(QString::number(a.id)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(a.name)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(a.value)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(a.worst)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(a.threshold)));
        table->setItem(row, 5, new QTableWidgetItem(QString::number(a.raw))); 
        ++row;
    }
    table->resizeColumnsToContents();
    layout->addWidget(table);

    QPushButton* closeBtn = new QPushButton(tr("关闭"), &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg.exec();
}

