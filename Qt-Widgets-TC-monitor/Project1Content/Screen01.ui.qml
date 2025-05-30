import QtQuick 2.15
import QtQuick.Controls 6.8
import QtQuick.Layouts 1.15

Item {
    id: monitorScreen
    width: 1920
    height: 1080

    // 这些属性将由 C++ 后端提供
    property string localDateTime: ""
    property string localDate: ""
    property string localTime: ""
    property string gitCommitHash: ""
    property string softwareVersion: ""
    property string localIpAddress: ""
    property int smartDetailVisibleIndex: -1
    property var disksModel: []

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        color: "#181818"
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 20
        anchors.bottomMargin: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: 20

            // 标题栏
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                Layout.minimumHeight: 70
                color: "#252525"
                radius: 10
                border.color: "#404040"
                border.width: 1

                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: "#252525"
                        }
                        GradientStop {
                            position: 1.0
                            color: "#1e1e1e"
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 20

                    // 左侧图标和标题
                    RowLayout {
                        spacing: 12

                        Rectangle {
                            width: 40
                            height: 40
                            color: "#0078D4"
                            radius: 8

                            Label {
                                anchors.centerIn: parent
                                text: "TC"
                                color: "white"
                                font.bold: true
                                font.pointSize: 14
                            }
                        }

                        ColumnLayout {
                            spacing: 2

                            Label {
                                text: "TC-client-control_panel"
                                color: "white"
                                font.bold: true
                                font.pointSize: 18
                            }

                            Label {
                                text: "系统监控控制面板 | 用户: dongge0210"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    // 右侧状态信息
                    RowLayout {
                        spacing: 15

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: "#4CAF50"
                        }

                        Label {
                            text: "系统运行中"
                            color: "#4CAF50"
                            font.pointSize: 10
                            font.bold: true
                        }

                        Rectangle {
                            width: 1
                            height: 25
                            color: "#404040"
                        }

                        Label {
                            text: monitorScreen.localDateTime
                            color: "#64B5F6"
                            font.bold: true
                            font.pointSize: 12
                        }
                    }
                }
            }

            // 第一行：CPU 和 内存
            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: 260
                spacing: 30

                // CPU 信息卡片
                Rectangle {
                    Layout.preferredWidth: 880
                    Layout.preferredHeight: 260
                    Layout.minimumHeight: 200
                    color: "#232323"
                    radius: 8
                    border.color: "#404040"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "处理器信息"
                                color: "white"
                                font.bold: true
                                font.pointSize: 14
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 55
                                height: 22
                                color: "#404040"
                                radius: 11

                                Label {
                                    anchors.centerIn: parent
                                    text: "CPU"
                                    color: "#64B5F6"
                                    font.bold: true
                                    font.pointSize: 9
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 2
                            rowSpacing: 12
                            columnSpacing: 40

                            Label {
                                text: "名称"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelCpuName
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                text: "物理核心"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelPhysicalCores
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "逻辑核心"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelLogicalCores
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "性能核心"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelPerformanceCores
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "能效核心"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelEfficiencyCores
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "CPU使用率"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelCpuUsage
                                text: "-- %"
                                color: "#4CAF50"
                                font.pointSize: 10
                                font.bold: true
                            }
                            Label {
                                text: "超线程"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelHyperThreading
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "虚拟化"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelVirtualization
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "CPU功率"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelCpuPower
                                text: "-- W"
                                color: "#FF9800"
                                font.pointSize: 10
                                font.bold: true
                            }
                        }
                    }
                }

                // 内存信息卡片
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 260
                    Layout.minimumHeight: 200
                    color: "#232323"
                    radius: 8
                    border.color: "#404040"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "内存信息"
                                color: "white"
                                font.bold: true
                                font.pointSize: 14
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 55
                                height: 22
                                color: "#404040"
                                radius: 11

                                Label {
                                    anchors.centerIn: parent
                                    text: "RAM"
                                    color: "#9C27B0"
                                    font.bold: true
                                    font.pointSize: 9
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 18

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "总内存"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelTotalMemory
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "已用内存"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelUsedMemory
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "可用内存"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelAvailableMemory
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "内存频率"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelMemoryFrequency
                                    text: "-- MHz"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "使用率"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelMemoryUsage
                                    text: "-- %"
                                    color: "#4CAF50"
                                    font.pointSize: 10
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            // 第二行：显卡 和 网络
            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: 280
                spacing: 30

                // 显卡信息卡片
                Rectangle {
                    Layout.preferredWidth: 880
                    Layout.preferredHeight: 280
                    Layout.minimumHeight: 220
                    color: "#232323"
                    radius: 8
                    border.color: "#404040"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "显卡信息"
                                color: "white"
                                font.bold: true
                                font.pointSize: 14
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 55
                                height: 22
                                color: "#404040"
                                radius: 11

                                Label {
                                    anchors.centerIn: parent
                                    text: "GPU"
                                    color: "#FF5722"
                                    font.bold: true
                                    font.pointSize: 9
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                text: "选择显卡"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            ComboBox {
                                id: gpuSelector
                                Layout.preferredWidth: 200
                                Layout.preferredHeight: 28
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 2
                            rowSpacing: 12
                            columnSpacing: 30

                            Label {
                                text: "名称"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuName
                                text: "无数据"
                                color: "white"
                                font.pointSize: 10
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                text: "专用显存"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuVram
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "共享内存"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuSharedMem
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "核心频率"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuCoreFreq
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "GPU功率"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuPower
                                text: "-- W"
                                color: "#FF9800"
                                font.pointSize: 10
                                font.bold: true
                            }
                            Label {
                                text: "驱动版本"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuDriverVersion
                                text: "--"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "状态"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuStatus
                                text: "无数据"
                                color: "white"
                                font.pointSize: 10
                            }
                            Label {
                                text: "温度"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            Label {
                                id: labelGpuTemp
                                text: "无数据"
                                color: "#2196F3"
                                font.pointSize: 10
                                font.bold: true
                            }
                        }
                    }
                }

                // 网络适配器卡片
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                    Layout.minimumHeight: 220
                    color: "#232323"
                    radius: 8
                    border.color: "#404040"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 15

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "网络适配器"
                                color: "white"
                                font.bold: true
                                font.pointSize: 14
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 55
                                height: 22
                                color: "#404040"
                                radius: 11

                                Label {
                                    anchors.centerIn: parent
                                    text: "NET"
                                    color: "#00BCD4"
                                    font.bold: true
                                    font.pointSize: 9
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                text: "选择适配器"
                                color: "#aaaaaa"
                                font.pointSize: 10
                            }
                            ComboBox {
                                id: networkSelector
                                Layout.preferredWidth: 200
                                Layout.preferredHeight: 28
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 18

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "名称"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelNetworkName
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "连接状态"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelNetworkStatus
                                    text: "--"
                                    color: "#4CAF50"
                                    font.pointSize: 10
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "IP地址"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelNetworkIp
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "MAC地址"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelNetworkMac
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15
                                Label {
                                    text: "速度"
                                    color: "#aaaaaa"
                                    font.pointSize: 10
                                    Layout.preferredWidth: 70
                                }
                                Label {
                                    id: labelNetworkSpeed
                                    text: "--"
                                    color: "white"
                                    font.pointSize: 10
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            // 磁盘信息（独占一行）
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(
                                            400, Math.max(
                                                200,
                                                monitorScreen.disksModel.length * 120 + 100))
                Layout.minimumHeight: 200
                Layout.maximumHeight: 500
                color: "#232323"
                radius: 8
                border.color: "#404040"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            text: "磁盘信息"
                            color: "white"
                            font.bold: true
                            font.pointSize: 14
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            width: 60
                            height: 22
                            color: "#404040"
                            radius: 11

                            Label {
                                anchors.centerIn: parent
                                text: "DISK"
                                color: "#8BC34A"
                                font.bold: true
                                font.pointSize: 9
                            }
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded

                        ColumnLayout {
                            width: parent.width - 15
                            spacing: 10

                            // 当没有磁盘数据时显示提示
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 60
                                color: "transparent"
                                visible: monitorScreen.disksModel.length === 0

                                Label {
                                    anchors.centerIn: parent
                                    text: "正在加载磁盘信息..."
                                    color: "#aaaaaa"
                                    font.pointSize: 14
                                }
                            }

                            Repeater {
                                model: monitorScreen.disksModel
                                delegate: ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    // 磁盘主卡片
                                    Rectangle {
                                        Layout.fillWidth: true
                                        color: "#2a2a2a"
                                        radius: 6
                                        height: 60
                                        border.color: "#404040"
                                        border.width: 1

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 15
                                            spacing: 20

                                            Label {
                                                text: model.name || ""
                                                color: "white"
                                                font.bold: true
                                                font.pointSize: 12
                                                Layout.preferredWidth: 80
                                            }
                                            Label {
                                                text: "类型: " + (model.type
                                                                || "")
                                                color: "#cccccc"
                                                font.pointSize: 10
                                                Layout.preferredWidth: 100
                                            }
                                            Label {
                                                text: "协议: " + (model.protocol
                                                                || "")
                                                color: "#cccccc"
                                                font.pointSize: 10
                                                Layout.preferredWidth: 100
                                            }
                                            Label {
                                                text: "容量: " + (model.totalSize
                                                                || "")
                                                color: "#cccccc"
                                                font.pointSize: 10
                                                Layout.preferredWidth: 120
                                            }
                                            Label {
                                                text: "SMART: " + (model.smart
                                                                   || "")
                                                color: model.smart === "正常" ? "#4CAF50" : model.smart === "警告" ? "#FF9800" : "#F44336"
                                                font.pointSize: 10
                                                font.bold: true
                                                Layout.preferredWidth: 100
                                            }

                                            Item {
                                                Layout.fillWidth: true
                                            }

                                            Button {
                                                text: "SMART详情"
                                                Layout.preferredWidth: 80
                                                Layout.preferredHeight: 28
                                                background: Rectangle {
                                                    color: parent.hovered ? "#404040" : "#353535"
                                                    radius: 4
                                                    border.color: "#505050"
                                                    border.width: 1
                                                }
                                                contentItem: Text {
                                                    text: parent.text
                                                    color: "white"
                                                    font.pointSize: 9
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                            }
                                        }
                                    }

                                    // 分区列表
                                    Repeater {
                                        model: model.partitions || []
                                        delegate: Rectangle {
                                            Layout.fillWidth: true
                                            height: 40
                                            color: "#272727"
                                            radius: 4
                                            border.color: "#404040"
                                            border.width: 1

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 10
                                                spacing: 15

                                                Label {
                                                    text: modelData.name || ""
                                                    color: "#64B5F6"
                                                    font.bold: true
                                                    font.pointSize: 10
                                                    Layout.preferredWidth: 50
                                                }
                                                Label {
                                                    text: "文件系统: " + (modelData.fileSystem
                                                                      || "")
                                                    color: "#bbbbbb"
                                                    font.pointSize: 9
                                                    Layout.preferredWidth: 120
                                                }
                                                Label {
                                                    text: "总容量: " + (modelData.totalSize
                                                                     || "")
                                                    color: "#bbbbbb"
                                                    font.pointSize: 9
                                                    Layout.preferredWidth: 110
                                                }
                                                Label {
                                                    text: "可用: " + (modelData.available
                                                                    || "")
                                                    color: "#bbbbbb"
                                                    font.pointSize: 9
                                                    Layout.preferredWidth: 100
                                                }
                                                Label {
                                                    property int percentValue: modelData.percent
                                                                               || 0
                                                    text: "使用率: " + (modelData.percent !== undefined ? modelData.percent + "%" : "--")
                                                    color: percentValue > 80 ? "#F44336" : (percentValue > 60 ? "#FF9800" : "#4CAF50")
                                                    font.pointSize: 9
                                                    font.bold: true
                                                    Layout.preferredWidth: 80
                                                }

                                                ProgressBar {
                                                    property real percentValue: (modelData.percent
                                                                                 || 0) / 100
                                                    value: percentValue
                                                    Layout.preferredWidth: 150
                                                    Layout.preferredHeight: 18

                                                    background: Rectangle {
                                                        color: "#1a1a1a"
                                                        radius: 9
                                                        border.color: "#404040"
                                                        border.width: 1
                                                    }

                                                    contentItem: Item {
                                                        Rectangle {
                                                            property int diskPercent: modelData.percent || 0
                                                            width: parent.parent.visualPosition
                                                                   * parent.width
                                                            height: parent.height
                                                            radius: 9
                                                            color: diskPercent > 80 ? "#F44336" : (diskPercent > 60 ? "#FF9800" : "#4CAF50")
                                                        }
                                                    }
                                                }

                                                Item {
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }
                                    }

                                    // 磁盘间的分隔线
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 1
                                        color: "#404040"
                                        Layout.topMargin: 5
                                        Layout.bottomMargin: 5
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 中间栏 - 系统信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                Layout.minimumHeight: 50
                color: "#232323"
                radius: 8
                border.color: "#404040"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 25

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "整机功率:"
                            color: "#aaaaaa"
                            font.pointSize: 10
                        }
                        Label {
                            id: labelTotalPower
                            text: "-- W"
                            color: "#FF9800"
                            font.pointSize: 10
                            font.bold: true
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 30
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "主板:"
                            color: "#aaaaaa"
                            font.pointSize: 10
                        }
                        Label {
                            id: labelMotherboardName
                            text: "--"
                            color: "white"
                            font.pointSize: 10
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 30
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "设备:"
                            color: "#aaaaaa"
                            font.pointSize: 10
                        }
                        Label {
                            id: labelDeviceName
                            text: "--"
                            color: "white"
                            font.pointSize: 10
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 30
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "系统:"
                            color: "#aaaaaa"
                            font.pointSize: 10
                        }
                        Label {
                            id: labelOsVersion
                            text: "--"
                            color: "white"
                            font.pointSize: 10
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            // 底栏 - 纯文字版本，去掉所有emoji图标
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                Layout.minimumHeight: 45
                color: "#1a1a1a"
                radius: 8
                border.color: "#404040"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 20

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "日期:"
                            color: "#aaaaaa"
                            font.pointSize: 9
                        }
                        Label {
                            id: labelLocalDate
                            text: monitorScreen.localDate || "--"
                            color: "#64B5F6"
                            font.pointSize: 10
                            font.bold: true
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 25
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "时间:"
                            color: "#aaaaaa"
                            font.pointSize: 9
                        }
                        Label {
                            id: labelLocalTime
                            text: monitorScreen.localTime || "--"
                            color: "#64B5F6"
                            font.pointSize: 10
                            font.bold: true
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 25
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "Git:"
                            color: "#aaaaaa"
                            font.pointSize: 9
                        }
                        Label {
                            id: labelGitCommitHash
                            text: monitorScreen.gitCommitHash || "--"
                            color: "#4CAF50"
                            font.pointSize: 9
                            font.family: "Consolas, monospace"
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 25
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "本机IP:"
                            color: "#aaaaaa"
                            font.pointSize: 9
                        }
                        Label {
                            id: labelLocalIpAddress
                            text: monitorScreen.localIpAddress || "--"
                            color: "#00BCD4"
                            font.pointSize: 9
                            font.bold: true
                            font.family: "Consolas, monospace"
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 25
                        color: "#404040"
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "版本:"
                            color: "#aaaaaa"
                            font.pointSize: 9
                        }
                        Label {
                            id: labelSoftwareVersion
                            text: monitorScreen.softwareVersion || "--"
                            color: "#FF9800"
                            font.pointSize: 9
                            font.bold: true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 8
                        Label {
                            text: "TC-Windows-Client"
                            color: "#888888"
                            font.pointSize: 9
                            font.italic: true
                        }
                    }
                }
            }
        }
    }
}
