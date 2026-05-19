import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import App 1.0

/**
 * ShareDialog — 文件分享弹窗
 * 选择有效期 + 分享形式后调用 ShareController 创建分享链接
 */
FluContentDialog {
    id: root

    title: "分享文件"
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "取消"
    positiveText: "创建链接"

    // ── 状态属性（驱动 contentDelegate 内控件） ──
    property string _fileId: ""
    property string _fileName: ""
    property int _validityIndex: 1      // 0=永久  1=7天  2=1天
    property bool _isPrivate: true

    // 外部调用入口
    function openDialog(fileInfo) {
        root._fileId = fileInfo.fileId;
        root._fileName = fileInfo.fileName;
        root._validityIndex = 1;
        root._isPrivate = true;
        root.open();
    }

    onPositiveClicked: {
        var days = root._validityIndex === 0 ? -1 : (root._validityIndex === 1 ? 7 : 1);
        ShareController.createShare(root._fileId, days, root._isPrivate);
    }

    contentDelegate: Component {
        Item {
            implicitHeight: contentCol.implicitHeight + 28

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 0

                // ── 头部：图标 + 文件名 ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 4
                    Layout.bottomMargin: 16
                    spacing: 14

                    // 分享图标徽章
                    Rectangle {
                        width: 44
                        height: 44
                        radius: 10
                        color: "#EEF1FD"
                        Layout.alignment: Qt.AlignVCenter

                        FluIcon {
                            anchors.centerIn: parent
                            iconSource: FluentIcons.Share
                            iconSize: 24
                            color: "#4F6BED"
                        }
                    }

                    Column {
                        spacing: 3
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true

                        FluText {
                            text: root._fileName
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#1a1a2e"
                            elide: Text.ElideMiddle
                            width: parent.width
                        }

                        FluText {
                            text: "创建分享链接并复制到剪贴板"
                            font.pixelSize: 12
                            color: "#888888"
                        }
                    }
                }

                // ── 分隔线 ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 16
                    color: "#efefef"
                }

                // ── 有效期选择 ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 16
                    spacing: 6

                    FluText {
                        text: "有效期"
                        font.pixelSize: 13
                        color: "#444444"
                    }

                    FluComboBox {
                        Layout.fillWidth: true
                        model: ["永久有效", "7 天有效", "1 天有效"]
                        currentIndex: root._validityIndex
                        onActivated: root._validityIndex = currentIndex
                    }
                }

                // ── 分享形式选择 ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 12
                    spacing: 6

                    FluText {
                        text: "分享形式"
                        font.pixelSize: 13
                        color: "#444444"
                    }

                    // 私密分享选项卡
                    Rectangle {
                        Layout.fillWidth: true
                        height: 48
                        radius: 6
                        color: root._isPrivate ? "#EEF1FD" : Qt.rgba(0, 0, 0, 0.02)
                        border.width: root._isPrivate ? 1.5 : 1
                        border.color: root._isPrivate ? "#4F6BED" : "#e0e0e0"

                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                        Behavior on border.color {
                            ColorAnimation {
                                duration: 150
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root._isPrivate = true
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 10

                            FluIcon {
                                iconSource: FluentIcons.Lock
                                iconSize: 16
                                color: root._isPrivate ? "#4F6BED" : "#999"
                            }

                            Column {
                                spacing: 1
                                Layout.fillWidth: true

                                FluText {
                                    text: "私密分享"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    color: root._isPrivate ? "#2c3e6b" : "#555"
                                }

                                FluText {
                                    text: "需要提取码才能访问"
                                    font.pixelSize: 11
                                    color: "#999"
                                }
                            }

                            // 选中指示器
                            Rectangle {
                                width: 18
                                height: 18
                                radius: 9
                                border.width: root._isPrivate ? 5 : 1.5
                                border.color: root._isPrivate ? "#4F6BED" : "#ccc"
                                color: "transparent"

                                Behavior on border.width {
                                    NumberAnimation {
                                        duration: 120
                                    }
                                }
                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: 120
                                    }
                                }
                            }
                        }
                    }

                    // 公开分享选项卡
                    Rectangle {
                        Layout.fillWidth: true
                        height: 48
                        radius: 6
                        color: !root._isPrivate ? "#EEF1FD" : Qt.rgba(0, 0, 0, 0.02)
                        border.width: !root._isPrivate ? 1.5 : 1
                        border.color: !root._isPrivate ? "#4F6BED" : "#e0e0e0"

                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                        Behavior on border.color {
                            ColorAnimation {
                                duration: 150
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root._isPrivate = false
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 10

                            FluIcon {
                                iconSource: FluentIcons.World
                                iconSize: 16
                                color: !root._isPrivate ? "#4F6BED" : "#999"
                            }

                            Column {
                                spacing: 1
                                Layout.fillWidth: true

                                FluText {
                                    text: "公开分享"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    color: !root._isPrivate ? "#2c3e6b" : "#555"
                                }

                                FluText {
                                    text: "任何人获得链接即可查看"
                                    font.pixelSize: 11
                                    color: "#999"
                                }
                            }

                            Rectangle {
                                width: 18
                                height: 18
                                radius: 9
                                border.width: !root._isPrivate ? 5 : 1.5
                                border.color: !root._isPrivate ? "#4F6BED" : "#ccc"
                                color: "transparent"

                                Behavior on border.width {
                                    NumberAnimation {
                                        duration: 120
                                    }
                                }
                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: 120
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
