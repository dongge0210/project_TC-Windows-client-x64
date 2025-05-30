// QtWidgetsTCmonitor.h
#pragma once

#include "stdafx.h"
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtGui/QPainter>
#include <QTimer>
#include <queue>
#include <map>
#include <vector>
#include <QString>
#include <QMap>
#include <QVector>
#include <QTreeWidget>
#include <QPushButton>
#include <QDialog>
#include <QTableWidget>
#include <QDateTime>
#include "../src/core/DataStruct/DataStruct.h"
#include "../src/core/utils/Logger.h"
#include "ui_QtWidgetsTCmonitor.h"

#ifndef MAX_DATA_POINTS
#define MAX_DATA_POINTS 1000
#endif

class QtWidgetsTCmonitor : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsTCmonitor(QWidget* parent = nullptr);
    ~QtWidgetsTCmonitor();

    void updateSystemInfo(const SystemInfo& sysInfo);
    void updateTemperatureData(const std::vector<std::pair<std::string, float>>& temperatures);

private slots:
    void updateCharts();
    void on_pushButton_clicked();
    void updateFromSharedMemory();
    void onGpuSelectionChanged(int index);
    void onNetworkSelectionChanged(int index);
    void updateLocalDateTime();

private:
    // UI setup
    void setupUI();
    void createCpuSection();
    void createMemorySection();
    void createGpuSection();
    void createTemperatureSection();
    void createDiskSection();
    void createNetworkSection();
    void updateGpuSelector();
    void updateNetworkSelector(int adapterCount, SharedMemoryBlock* pBuffer);
    void updateDiskInfo(SharedMemoryBlock* pBuffer);
    void UpdateDiskInfoUI();
    void updateDiskTreeWidget();
    void showSmartDetails(const QString& diskName);

    QString formatSize(uint64_t bytes);
    QString formatPercentage(double value);
    QString formatTemperature(double value);
    QString formatFrequency(double value);

    QTimer* updateTimer = nullptr;

    QGroupBox* cpuGroupBox = nullptr;
    QGroupBox* memoryGroupBox = nullptr;
    QGroupBox* gpuGroupBox = nullptr;
    QGroupBox* temperatureGroupBox = nullptr;
    QGroupBox* diskGroupBox = nullptr;
    QGroupBox* networkGroupBox = nullptr;

    QChart* cpuTempChart = nullptr;
    QChartView* cpuTempChartView = nullptr;
    QLineSeries* cpuTempSeries = nullptr;
    QChart* gpuTempChart = nullptr;
    QChartView* gpuTempChartView = nullptr;
    QLineSeries* gpuTempSeries = nullptr;

    QComboBox* gpuSelector = nullptr;
    std::vector<int> gpuIndices;
    int currentGpuIndex = 0;
    QLabel* gpuDriverVersionLabel = nullptr;
    int cachedGpuCount = 0;

    QWidget* networkContainer = nullptr;
    QComboBox* networkSelector = nullptr;
    QVector<int> networkIndices;
    int currentNetworkIndex = 0;
    QLabel* networkNameLabel = nullptr;
    QLabel* networkMacLabel = nullptr;
    QLabel* networkSpeedLabel = nullptr;
    QLabel* networkStatusLabel = nullptr;
    QLabel* networkIpLabel = nullptr;

    QMap<QString, QLabel*> infoLabels;

    std::queue<float> cpuTempHistory;
    std::queue<float> gpuTempHistory;
    SystemInfo currentSysInfo;

    Ui_QtWidgetsTCmonitorClass* ui = nullptr;
    QLabel* labelLocalDateTime = nullptr; // 新增：用于本地时间显示
};
