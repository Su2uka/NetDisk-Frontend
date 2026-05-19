import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI

Rectangle {
    id: root

    // ── 属性 ──
    height: contentColumn.implicitHeight + 20
    radius: 12
    color: "#ffffff"
    border.color: "#e8eaef"
    border.width: 0.5

    // ── 子对象 ──
    FluShadow {
        radius: 12
    }

    // 数据模型
    ListModel {
        id: recentModel
        ListElement {
            fileName: "需求规格说明书_v1.2.pdf"
            fileIcon: "qrc:/file_type/res/file_type/ft-pdf.svg"
            fileTime: "Today, 14:59"
        }
        ListElement {
            fileName: "首页UI设计稿_终版.png"
            fileIcon: "qrc:/file_type/res/file_type/ft-image.svg"
            fileTime: "Today, 14:06"
        }
        ListElement {
            fileName: "首页UI设计稿_终版.psd"
            fileIcon: "qrc:/file_type/res/file_type/ft-psd.svg"
            fileTime: "Today, 14:54"
        }
        ListElement {
            fileName: "项目进度表.xlsx"
            fileIcon: "qrc:/file_type/res/file_type/ft-excel.svg"
            fileTime: "Yesterday, 18:30"
        }
        ListElement {
            fileName: "会议纪要_0223.docx"
            fileIcon: "qrc:/file_type/res/file_type/ft-word.svg"
            fileTime: "Yesterday, 10:15"
        }
        ListElement {
            fileName: "会议纪要_0223.docx"
            fileIcon: "qrc:/file_type/res/file_type/ft-word.svg"
            fileTime: "Yesterday, 10:15"
        }
        ListElement {
            fileName: "会议纪要_0223.docx"
            fileIcon: "qrc:/file_type/res/file_type/ft-word.svg"
            fileTime: "Yesterday, 10:15"
        }
    }

    Column {
        id: contentColumn
        anchors {
            fill: parent
            leftMargin: 20
            rightMargin: 20
            topMargin: 16
            bottomMargin: 16
        }

        FluText {
            text: "最近文件"
            font.pixelSize: 16
            font.bold: true
            color: "#1a1a2e"
            bottomPadding: 8
        }

        ListView {
            id: fileListView
            width: parent.width
            height: contentHeight
            interactive: false
            model: recentModel

            delegate: ItemDelegate {
                id: delegateItem
                width: fileListView.width
                height: 52
                padding: 0

                background: Rectangle {
                    anchors.fill: parent
                    anchors.bottomMargin: 2
                    anchors.topMargin: 2
                    color: "#f5f5f5"
                    radius: 6
                    opacity: delegateItem.hovered ? 1.0 : 0.0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 120
                        }
                    }
                }

                contentItem: RowLayout {
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    anchors.bottomMargin: 4
                    spacing: 12

                    Image {
                        source: model.fileIcon
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(28, 28)
                    }

                    FluText {
                        text: model.fileName
                        font.pixelSize: 14
                        color: "#333333"
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    FluText {
                        text: model.fileTime
                        font.pixelSize: 13
                        color: "#999999"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                onClicked: console.log("[RecentFiles] Clicked:", model.fileName)

                // 底部分割线
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 4
                    height: 1
                    color: "#f0f0f0"
                    visible: index < recentModel.count - 1
                }
            }
        }
    }
}
