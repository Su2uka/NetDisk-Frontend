import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI
import App 1.0

Rectangle {
    id: root

    // ── 属性 ──
    property real usagePercent: UserController.totalCapacity > 0 ? UserController.usedCapacity / UserController.totalCapacity : 0
    property string usedText: "已使用 " + root.formatPercent(root.usagePercent)
    property string totalText: root.formatBytes(UserController.usedCapacity) + " / " + root.formatBytes(UserController.totalCapacity)

    height: 120
    radius: 12
    color: "#ffffff"
    border.color: "#e8eaef"
    border.width: 0.5

    // ── 函数 ──
    function formatBytes(bytes) {
        if (bytes <= 0)
            return "0 B";
        var units = ["B", "KB", "MB", "GB", "TB"];
        var i = Math.floor(Math.log(bytes) / Math.log(1024));
        if (i >= units.length)
            i = units.length - 1;
        var value = bytes / Math.pow(1024, i);
        return value.toFixed(value >= 100 ? 0 : (value >= 10 ? 1 : 2)) + " " + units[i];
    }

    function formatPercent(ratio) {
        return (ratio * 100).toFixed(1) + "%";
    }

    // ── 子对象 ──
    FluShadow {
        radius: 12
    }

    Column {
        anchors {
            fill: parent
            leftMargin: 20
            rightMargin: 20
            topMargin: 18
            bottomMargin: 18
        }
        spacing: 12

        // 标题
        FluText {
            text: "存储概览"
            font.pixelSize: 16
            font.bold: true
            color: "#1a1a2e"
        }

        // 进度条
        ProgressBar {
            id: storageBar
            width: parent.width
            height: 10
            from: 0
            to: 1
            value: root.usagePercent

            background: Rectangle {
                implicitHeight: 10
                radius: 5
                color: "#eef0f4"
            }

            contentItem: Item {
                implicitHeight: 10

                Rectangle {
                    width: storageBar.visualPosition * parent.width
                    height: parent.height
                    radius: 5

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop {
                            position: 0.0
                            color: root.usagePercent < 0.8 ? "#4facfe" : "#ff6b6b"
                        }
                        GradientStop {
                            position: 1.0
                            color: root.usagePercent < 0.8 ? "#00f2fe" : "#ee5a24"
                        }
                    }

                    Behavior on width {
                        NumberAnimation {
                            duration: 600
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }

        // 底部信息行
        RowLayout {
            width: parent.width

            FluText {
                text: root.usedText
                font.pixelSize: 13
                color: "#6b7280"
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }

            FluText {
                text: root.totalText
                font.pixelSize: 13
                color: "#6b7280"
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }
}
