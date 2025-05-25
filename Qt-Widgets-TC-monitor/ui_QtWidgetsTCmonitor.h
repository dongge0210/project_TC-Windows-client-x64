/********************************************************************************
** Form generated from reading UI file 'QtWidgetsTCmonitor.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTWIDGETSTCMONITOR_H
#define UI_QTWIDGETSTCMONITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QLabel>
#include "../src/core/DataStruct/SharedMemoryManager.h"
#include "../src/core/DataStruct/DataStruct.h" // Include GPUInfo definition

QT_BEGIN_NAMESPACE

class Ui_QtWidgetsTCmonitorClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *pushButton;
    QTableWidget *diskTable;
    QTreeWidget *treeWidgetDiskInfo;
    QMenuBar *menuBar;
    QStatusBar *statusBar;

    // Add GPU info labels
    QLabel *label_gpu_name;
    QLabel *label_gpu_status;
    QLabel *label_gpu_temp;

    void setupUi(QMainWindow *QtWidgetsTCmonitorClass)
    {
        if (QtWidgetsTCmonitorClass->objectName().isEmpty())
            QtWidgetsTCmonitorClass->setObjectName("QtWidgetsTCmonitorClass");
        QtWidgetsTCmonitorClass->resize(800, 600);
        centralWidget = new QWidget(QtWidgetsTCmonitorClass);
        centralWidget->setObjectName("centralWidget");
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName("verticalLayout");
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName("pushButton");
        pushButton->setMinimumSize(QSize(100, 30));

        verticalLayout->addWidget(pushButton);

        // Add GPU info labels to the layout
        label_gpu_name = new QLabel(centralWidget);
        label_gpu_name->setObjectName("label_gpu_name");
        label_gpu_name->setText("无数据");
        verticalLayout->addWidget(label_gpu_name);

        label_gpu_status = new QLabel(centralWidget);
        label_gpu_status->setObjectName("label_gpu_status");
        label_gpu_status->setText("未支持");
        verticalLayout->addWidget(label_gpu_status);

        label_gpu_temp = new QLabel(centralWidget);
        label_gpu_temp->setObjectName("label_gpu_temp");
        label_gpu_temp->setText("无数据");
        verticalLayout->addWidget(label_gpu_temp);

        diskTable = new QTableWidget(centralWidget);
        diskTable->setObjectName("diskTable");
        diskTable->setColumnCount(6);
        diskTable->setHorizontalHeaderLabels(QStringList() << "盘符" << "卷标" << "文件系统" << "总容量" << "已用空间" << "可用空间");

        verticalLayout->addWidget(diskTable);

        treeWidgetDiskInfo = new QTreeWidget(centralWidget);
        treeWidgetDiskInfo->setObjectName("treeWidgetDiskInfo");

        verticalLayout->addWidget(treeWidgetDiskInfo);

        // 修正：设置中心部件
        QtWidgetsTCmonitorClass->setCentralWidget(centralWidget);
    }

    // Update UI to handle/display 'no data' or error states for GPU info
    void updateGPUInfo(const GPUInfo& info) {
        if (!info.available) {
            label_gpu_name->setText("无数据");
            label_gpu_status->setText("未支持");
            label_gpu_temp->setText("无数据");
            // ...set other fields to "无数据" or "未支持"...
        } else {
            label_gpu_name->setText(QString::fromStdString(info.name));
            label_gpu_status->setText(QString::fromStdString(info.status));
            label_gpu_temp->setText(QString::number(info.temperature) + " °C");
            // ...set other fields...
        }
    }
};

QT_END_NAMESPACE

#endif // UI_QTWIDGETSTCMONITOR_H
