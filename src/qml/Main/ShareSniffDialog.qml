import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import FluentUI 1.0
import App 1.0

/**
 * ShareSniffDialog — 剪贴板分享口令嗅探弹窗
 * 检测到用户复制了分享口令并切回窗口时弹出
 */
FluContentDialog {
    id: root

    title: ""
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "忽略"
    positiveText: "立即查看"

    // ── 外部传入的口令数据 ──
    property string _shareKey: ""
    property string _extractCode: ""

    // ── 验证中状态 ──
    property bool _verifying: false

    function openDialog(shareKey, extractCode) {
        root._shareKey = shareKey;
        root._extractCode = extractCode;
        root._verifying = false;
        root.open();
    }

    onPositiveClicked: {
        root._verifying = true;
        ClipboardController.openShare(root._shareKey, root._extractCode);
    }

    onNegativeClicked: {
        ClipboardController.dismissShare(root._shareKey);
    }

    contentDelegate: Component {
        Item {
            implicitHeight: contentCol.implicitHeight + 28

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 0

                // ── 头部：图标徽章 + 标题 ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 4
                    Layout.bottomMargin: 16
                    spacing: 14

                    // 嗅探图标徽章
                    Rectangle {
                        width: 48
                        height: 48
                        radius: 12
                        Layout.alignment: Qt.AlignVCenter

                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#E8EDFF" }
                            GradientStop { position: 1.0; color: "#DCE3FF" }
                        }

                        FluIcon {
                            anchors.centerIn: parent
                            iconSource: FluentIcons.Link
                            iconSize: 24
                            color: "#4F6BED"
                        }

                        // 右上角小圆点脉冲
                        Rectangle {
                            width: 12
                            height: 12
                            radius: 6
                            color: "#4CAF50"
                            border.width: 2
                            border.color: "white"
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.rightMargin: -2
                            anchors.topMargin: -2

                            SequentialAnimation on scale {
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 1.3; duration: 600; easing.type: Easing.InOutQuad }
                                NumberAnimation { from: 1.3; to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
                            }
                        }
                    }

                    Column {
                        spacing: 3
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true

                        FluText {
                            text: "检测到分享口令"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            color: "#1a1a2e"
                        }

                        FluText {
                            text: "来自剪贴板的分享邀请"
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

                // ── 口令信息卡 ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: codeContent.implicitHeight + 28
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 12
                    radius: 10
                    color: "#F7F8FC"
                    border.width: 1
                    border.color: "#ECEDF3"

                    ColumnLayout {
                        id: codeContent
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        // 口令行
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            FluIcon {
                                iconSource: FluentIcons.Permissions
                                iconSize: 15
                                color: "#666666"
                            }

                            FluText {
                                text: "口令"
                                font.pixelSize: 12
                                color: "#888888"
                                Layout.preferredWidth: 42
                            }

                            // 口令值高亮显示
                            Rectangle {
                                Layout.fillWidth: true
                                height: 28
                                radius: 6
                                color: "#EEF1FD"
                                border.width: 1
                                border.color: "#D8DFFB"

                                FluText {
                                    anchors.centerIn: parent
                                    text: root._shareKey
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    font.letterSpacing: 2
                                    color: "#4F6BED"
                                }
                            }
                        }

                        // 提取码行（仅在有提取码时显示）
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: root._extractCode.length > 0

                            FluIcon {
                                iconSource: FluentIcons.Lock
                                iconSize: 15
                                color: "#666666"
                            }

                            FluText {
                                text: "提取码"
                                font.pixelSize: 12
                                color: "#888888"
                                Layout.preferredWidth: 42
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 28
                                radius: 6
                                color: "#FFF8E1"
                                border.width: 1
                                border.color: "#FFE0B2"

                                FluText {
                                    anchors.centerIn: parent
                                    text: root._extractCode
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    font.letterSpacing: 2
                                    color: "#F57C00"
                                }
                            }
                        }
                    }
                }

                // ── 提示文字 ──
                FluText {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 4
                    text: root._extractCode.length > 0
                          ? "提取码已自动填入，点击「立即查看」一键打开"
                          : "点击「立即查看」一键打开分享内容"
                    font.pixelSize: 11
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
