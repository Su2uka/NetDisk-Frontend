import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../../Common"

// ── 传输列表主页面 ──
Item {
    id: root

    property int currentTab: 0   // 0=上传, 1=下载, 2=已完成

    Column {
        anchors.fill: parent
        spacing: 0

        // ── 顶部标题 + Tab 栏 ──
        Rectangle {
            width: parent.width
            height: 56
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                FluText {
                    text: "传输列表"
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    color: "#222222"
                }

                // 复用 PillTabBar 组件
                PillTabBar {
                    tabLabels: ["上传", "下载", "已完成"]
                    currentIndex: root.currentTab
                    onCurrentIndexChanged: root.currentTab = currentIndex
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 8
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // 分割线
        Rectangle {
            width: parent.width
            height: 1
            color: "#e8e8e8"
        }

        // ── 内容区域 ──
        Item {
            id: item
            width: parent.width
            height: parent.height - 57

            property bool loaded0: false
            property bool loaded1: false
            property bool loaded2: false

            Loader {
                anchors.fill: parent
                active: root.currentTab === 0 || item.loaded0
                visible: root.currentTab === 0
                source: "TransferUploadList.qml"
                onLoaded: item.loaded0 = true
            }

            Loader {
                anchors.fill: parent
                active: root.currentTab === 1 || item.loaded1
                visible: root.currentTab === 1
                source: "TransferDownloadList.qml"
                onLoaded: item.loaded1 = true
            }

            Loader {
                anchors.fill: parent
                active: root.currentTab === 2 || item.loaded2
                visible: root.currentTab === 2
                source: "TransferCompletedList.qml"
                onLoaded: item.loaded2 = true
            }
        }
    }
}
