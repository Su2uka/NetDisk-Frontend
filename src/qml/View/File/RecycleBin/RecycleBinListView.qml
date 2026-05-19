import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

Column {
    id: root
    spacing: 0

    // ── 属性 ──
    required property var fileModel
    property bool hasSelection: false

    // ── 信号 ──
    signal toggleSelection(int index)
    signal contextMenuRequested(int index)

    // ── 函数 ──
    function rowBgColor(selected, hovered) {
        if (selected)
            return Qt.rgba(0, 0, 0, 0.04);
        if (hovered)
            return Qt.rgba(0, 0, 0, 0.02);
        return "transparent";
    }

    // ── 子对象 ──

    // 表头
    Rectangle {
        width: parent.width
        height: 40
        color: "transparent"

        Rectangle {
            width: parent.width - 40
            height: 1
            color: "#eeeeee"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            width: parent.width - 40
            height: 1
            color: "#eeeeee"
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            Item {
                width: 44
                height: 1
            }

            FluText {
                text: "名称"
                color: "#888888"
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            FluText {
                text: "删除时间"
                color: "#888888"
                font.pixelSize: 12
                Layout.preferredWidth: 200
            }

            FluText {
                text: "大小"
                color: "#888888"
                font.pixelSize: 12
                Layout.preferredWidth: 120
            }
        }
    }

    // 列表数据
    ListView {
        id: fileList
        width: parent.width
        height: parent.height - 40
        clip: true
        model: root.fileModel
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOff
        }

        delegate: Item {
            width: fileList.width
            height: 50

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                color: root.rowBgColor(model.selected, rowHover.hovered)
                radius: 6

                HoverHandler {
                    id: rowHover
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#e8e8e8"
                    anchors.bottom: parent.bottom
                    visible: index < root.fileModel.count - 1
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 16

                    Item {
                        width: 32
                        height: parent.height

                        FluCheckBox {
                            anchors.centerIn: parent
                            visible: root.hasSelection || rowHover.hovered
                            checked: model.selected
                            size: 16
                            clickListener: function () {
                                root.toggleSelection(index);
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Image {
                            source: model.fileIcon
                            width: 24
                            height: 24
                            sourceSize: Qt.size(24, 24)
                        }

                        FluText {
                            text: model.fileName
                            font.pixelSize: 13
                            color: "#222222"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    FluText {
                        text: model.deletedAt
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 200
                    }

                    FluText {
                        text: model.fileSizeStr
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 120
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: function (mouse) {
                        if (mouse.button === Qt.RightButton)
                            root.contextMenuRequested(index);
                    }
                }


            }
        }
    }
}
