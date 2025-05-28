#include "stdafx.h"
#include "QtWidgetsTCmonitor.h"
#include "ui_QtWidgetsTCmonitor.h"
#include <windows.h>
#include "../src/core/DataStruct/SharedMemoryManager.h"
#include "../src/core/Utils/WinUtils.h"
#include "../src/core/disk/DiskInfo.h"

#include <QtWidgets>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QSplitter>
#include <QTreeWidget>
#include <QScrollArea>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <map>
#include <queue>
#include <vector>
#include <string>

// 最大温度历史点数
#define MAX_DATA_POINTS 1000

QtWidgetsTCmonitor::QtWidgetsTCmonitor(QWidget* parent)
    : QMainWindow(parent)
{
    ui = new Ui_QtWidgetsTCmonitorClass();
    ui->setupUi(this);

    // 让磁盘信息区可拖拽高度
    QSplitter* splitterDisk = findChild<QSplitter*>("splitterDisk");
    if (splitterDisk) {
        splitterDisk->setChildrenCollapsible(false);
        splitterDisk->setStretchFactor(0, 1);
    }

    // 设置磁盘信息区QTreeWidget可上下滑动
    QTreeWidget* treeWidgetDiskInfo = findChild<QTreeWidget*>("treeWidgetDiskInfo");
    if (treeWidgetDiskInfo) {
        treeWidgetDiskInfo->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        treeWidgetDiskInfo->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        treeWidgetDiskInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    // 本地时间显示定时器
    labelLocalDateTime = findChild<QLabel*>("labelLocalDateTime");
    if (labelLocalDateTime) {
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &QtWidgetsTCmonitor::updateLocalDateTime);
        timer->start(1000);
        updateLocalDateTime();
    }

    // 其他初始化
    setWindowTitle(tr("系统硬件监视器"));
    resize(1000, 700);

    if (!SharedMemoryManager::IsSharedMemoryInitialized()) {
        if (!SharedMemoryManager::InitSharedMemory()) {
            QMessageBox::warning(this, tr("警告"),
                tr("无法连接到共享内存，系统数据将不会更新。\n错误: %1")
                    .arg(QString::fromStdString(SharedMemoryManager::GetSharedMemoryError())));
        }
    }

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &QtWidgetsTCmonitor::updateFromSharedMemory);
    updateTimer->start(1000);

    // 初始化温度曲线
    setupTemperatureCharts();

    // 其它自定义控件成员初始化
    gpuSelector = findChild<QComboBox*>("gpuSelector");
    if (gpuSelector) connect(gpuSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtWidgetsTCmonitor::onGpuSelectionChanged);

    networkSelector = findChild<QComboBox*>("networkSelector");
    if (networkSelector) connect(networkSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtWidgetsTCmonitor::onNetworkSelectionChanged);
}

QtWidgetsTCmonitor::~QtWidgetsTCmonitor()
{
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    delete ui;
}

void QtWidgetsTCmonitor::updateLocalDateTime()
{
    if (!labelLocalDateTime) return;
    QDateTime now = QDateTime::currentDateTime();
    labelLocalDateTime->setText(now.toString("yyyy-MM-dd HH:mm:ss"));
}

void QtWidgetsTCmonitor::setupTemperatureCharts()
{
    // CPU温度曲线
    cpuTempChart = new QtCharts::QChart();
    cpuTempChart->setTitle(tr("CPU温度历史"));
    cpuTempSeries = new QtCharts::QLineSeries();
    cpuTempChart->addSeries(cpuTempSeries);

    QtCharts::QValueAxis* axisX = new QtCharts::QValueAxis();
    axisX->setRange(0, MAX_DATA_POINTS);
    axisX->setLabelFormat("%d");
    axisX->setTitleText(tr("时间 (秒)"));
    QtCharts::QValueAxis* axisY = new QtCharts::QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d");
    axisY->setTitleText(tr("温度 (°C)"));

    cpuTempChart->addAxis(axisX, Qt::AlignBottom);
    cpuTempChart->addAxis(axisY, Qt::AlignLeft);
    cpuTempSeries->attachAxis(axisX);
    cpuTempSeries->attachAxis(axisY);

    cpuTempChartView = new QtCharts::QChartView(cpuTempChart, this);
    cpuTempChartView->setRenderHint(QPainter::Antialiasing);

    // GPU温度曲线
    gpuTempChart = new QtCharts::QChart();
    gpuTempChart->setTitle(tr("GPU温度历史"));
    gpuTempSeries = new QtCharts::QLineSeries();
    gpuTempChart->addSeries(gpuTempSeries);

    QtCharts::QValueAxis* axisX2 = new QtCharts::QValueAxis();
    axisX2->setRange(0, MAX_DATA_POINTS);
    axisX2->setLabelFormat("%d");
    axisX2->setTitleText(tr("时间 (秒)"));
    QtCharts::QValueAxis* axisY2 = new QtCharts::QValueAxis();
    axisY2->setRange(0, 100);
    axisY2->setLabelFormat("%d");
    axisY2->setTitleText(tr("温度 (°C)"));

    gpuTempChart->addAxis(axisX2, Qt::AlignBottom);
    gpuTempChart->addAxis(axisY2, Qt::AlignLeft);
    gpuTempSeries->attachAxis(axisX2);
    gpuTempSeries->attachAxis(axisY2);

    gpuTempChartView = new QtCharts::QChartView(gpuTempChart, this);
    gpuTempChartView->setRenderHint(QPainter::Antialiasing);

    // 将两个曲线加到温度分区（你可以在UI里放一个占位Widget用于布局，也可以用findChild定位QVBoxLayout）
    QGroupBox* temperatureGroupBox = findChild<QGroupBox*>("temperatureGroupBox");
    if (temperatureGroupBox) {
        QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(temperatureGroupBox->layout());
        if (layout) {
            QSplitter* tempSplitter = new QSplitter(Qt::Horizontal, this);
            tempSplitter->addWidget(cpuTempChartView);
            tempSplitter->addWidget(gpuTempChartView);
            layout->addWidget(tempSplitter);
        }
    }
}

// ========== 内存/CPU/GPU/网络/磁盘/温度数据刷新与辅助 ==========

void QtWidgetsTCmonitor::updateFromSharedMemory()
{
    SharedMemoryBlock* pBuffer = SharedMemoryManager::GetBuffer();
    if (!pBuffer) return;

    EnterCriticalSection(&pBuffer->lock);

    // 假定SystemInfo、GPUInfo等结构体你已经定义，并有合适的转换
    SystemInfo sysInfo;
    sysInfo.cpuName = WinUtils::WstringToQString(std::wstring(pBuffer->cpuName)).toStdString();
    sysInfo.physicalCores = pBuffer->physicalCores;
    sysInfo.logicalCores = pBuffer->logicalCores;
    sysInfo.performanceCores = pBuffer->performanceCores;
    sysInfo.efficiencyCores = pBuffer->efficiencyCores;
    sysInfo.cpuUsage = pBuffer->cpuUsage;
    sysInfo.cpuPower = pBuffer->cpuPower;
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
        gpu.power = pBuffer->gpus[i].power;
        gpu.driverVersion = WinUtils::WstringToQString(std::wstring(pBuffer->gpus[i].driverVersion)).toStdWString();
        gpu.temperature = pBuffer->gpus[i].temperature;
        sysInfo.gpus.push_back(gpu);
    }

    sysInfo.temperatures.clear();
    for (int i = 0; i < pBuffer->tempCount; ++i) {
        sysInfo.temperatures.push_back({
            WinUtils::WstringToQString(std::wstring(pBuffer->temperatures[i].sensorName)).toStdString(),
            pBuffer->temperatures[i].temperature
        });
    }

    LeaveCriticalSection(&pBuffer->lock);

    updateSystemInfo(sysInfo);
    updateCharts();
    updateDiskTreeWidget();
}

// ====== 界面标签赋值（示例，细节可按需补充） ======
void QtWidgetsTCmonitor::updateSystemInfo(const SystemInfo& sysInfo)
{
    // CPU
    setLabel("labelCpuName", QString::fromStdString(sysInfo.cpuName));
    setLabel("labelPhysicalCores", QString::number(sysInfo.physicalCores));
    setLabel("labelLogicalCores", QString::number(sysInfo.logicalCores));
    setLabel("labelPerformanceCores", QString::number(sysInfo.performanceCores));
    setLabel("labelEfficiencyCores", QString::number(sysInfo.efficiencyCores));
    setLabel("labelCpuUsage", QString::number(sysInfo.cpuUsage, 'f', 1) + "%");
    setLabel("labelCpuPower", QString::number(sysInfo.cpuPower, 'f', 1) + " W");
    setLabel("labelHyperThreading", sysInfo.hyperThreading ? tr("是") : tr("否"));
    setLabel("labelVirtualization", sysInfo.virtualization ? tr("是") : tr("否"));

    // 内存
    setLabel("labelTotalMemory", formatSize(sysInfo.totalMemory));
    setLabel("labelUsedMemory", formatSize(sysInfo.usedMemory));
    setLabel("labelAvailableMemory", formatSize(sysInfo.availableMemory));
    setLabel("labelMemoryFrequency", QString("%1 MHz").arg(sysInfo.memoryFrequency));
    double memP = sysInfo.totalMemory ? 100.0 * sysInfo.usedMemory / sysInfo.totalMemory : 0;
    setLabel("labelMemoryUsage", QString::number(memP, 'f', 1) + "%");

    // GPU（仅显示当前选中）
    int gpuIdx = gpuSelector ? gpuSelector->currentIndex() : 0;
    if (gpuIdx >= 0 && gpuIdx < int(sysInfo.gpus.size())) {
        const GPUInfo& g = sysInfo.gpus[gpuIdx];
        setLabel("labelGpuName", QString::fromStdString(g.name));
        setLabel("labelGpuVram", formatSize(g.vram));
        setLabel("labelGpuSharedMem", formatSize(g.sharedMemory));
        setLabel("labelGpuCoreFreq", QString::number(g.coreClock) + " MHz");
        setLabel("labelGpuPower", QString::number(g.power, 'f', 1) + " W");
        setLabel("labelGpuDriverVersion", QString::fromStdWString(g.driverVersion));
        setLabel("labelGpuTemp", QString::number(g.temperature, 'f', 1) + "°C");
    } else {
        setLabel("labelGpuName", tr("无数据"));
        setLabel("labelGpuVram", "--");
        setLabel("labelGpuSharedMem", "--");
        setLabel("labelGpuCoreFreq", "--");
        setLabel("labelGpuPower", "--");
        setLabel("labelGpuDriverVersion", "--");
        setLabel("labelGpuTemp", "--");
    }

    // 温度
    for (const auto& t : sysInfo.temperatures) {
        if (t.first.contains("CPU")) setLabel("labelCpuTemp", QString::number(t.second, 'f', 1) + "°C");
        if (t.first.contains("GPU")) setLabel("labelGpuTemp", QString::number(t.second, 'f', 1) + "°C");
    }
}

void QtWidgetsTCmonitor::setLabel(const char* name, const QString& value)
{
    QLabel* lbl = findChild<QLabel*>(name);
    if (lbl) lbl->setText(value);
}

// ========== 温度历史曲线刷新 ==========
void QtWidgetsTCmonitor::updateCharts()
{
    // 假定你有当前温度历史队列成员变量
    // 这里只做简单示例
    // 实际工程建议将温度数据push到queue，再刷新series
    // 此处略
}

// ========== 磁盘信息刷新 ==========
void QtWidgetsTCmonitor::updateDiskTreeWidget()
{
    QTreeWidget* tree = findChild<QTreeWidget*>("treeWidgetDiskInfo");
    if (!tree) return;
    tree->clear();

    auto disks = DiskInfo::GetAllPhysicalDisks();
    for (const auto& d : disks) {
        QTreeWidgetItem* item = new QTreeWidgetItem(tree);
        item->setText(0, QString::fromStdString(d.model.empty() ? d.name : d.model));
        item->setText(1, QString::fromStdString(d.type));
        item->setText(2, QString::fromStdString(d.protocol));
        item->setText(3, QString::fromStdString(DiskInfo::FormatSize(d.totalSize)));
        item->setText(4, ""); // 已用空间和使用率可后续补全
        item->setText(5, "");
        item->setText(6, QString::fromStdString(d.smartStatus));
        QPushButton* btn = new QPushButton(tr("SMART详情"), tree);
        tree->setItemWidget(item, 6, btn);
        connect(btn, &QPushButton::clicked, this, [this, diskId = d.name]() {
            showSmartDetails(QString::fromStdString(diskId));
        });
    }
    tree->expandAll();
    for(int i = 0; i < tree->columnCount(); ++i)
        tree->resizeColumnToContents(i);
}

// ========== GPU/网络选择 ==========
void QtWidgetsTCmonitor::onGpuSelectionChanged(int index)
{
    Q_UNUSED(index)
    updateFromSharedMemory();
}
void QtWidgetsTCmonitor::onNetworkSelectionChanged(int index)
{
    Q_UNUSED(index)
    updateFromSharedMemory();
}

// ========== SMART详情弹窗 ==========
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

// ========== 工具函数 ==========
QString QtWidgetsTCmonitor::formatSize(uint64_t bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;
    QString unit;
    double size;
    if (bytes >= TB) { size = bytes / TB; unit = "TB"; }
    else if (bytes >= GB) { size = bytes / GB; unit = "GB"; }
    else if (bytes >= MB) { size = bytes / MB; unit = "MB"; }
    else if (bytes >= KB) { size = bytes / KB; unit = "KB"; }
    else { size = bytes; unit = "B"; }
    return QString("%1 %2").arg(size, 0, 'f', 2).arg(unit);
}
