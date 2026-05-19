import QtQuick 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import App 1.0

/**
 * CreateFolderDialog — 新建文件夹弹窗
 *
 * 用法：CreateFolderDialog { id: dlg }
 *       dlg.open()
 */
FluContentDialog {
    id: root
    title: "新建文件夹"
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "取消"
    positiveText: "创建"

    property var _textBox: null

    onPositiveClicked: {
        if (root._textBox) {
            var name = root._textBox.text.trim();
            if (name.length > 0) {
                FileController.createFolder(name);
            }
        }
    }

    onOpened: {
        // contentDelegate 内部的 Component.onCompleted 会设置 _textBox
        if (root._textBox) {
            root._textBox.text = "新建文件夹";
            root._textBox.forceActiveFocus();
            root._textBox.selectAll();
        }
    }

    contentDelegate: Component {
        Item {
            implicitHeight: contentCol.implicitHeight + 28

            Component.onCompleted: root._textBox = nameInput

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 0

                // ── 图标 + 描述 ──
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 4
                    Layout.bottomMargin: 16
                    spacing: 14

                    // 文件夹图标背景
                    Rectangle {
                        width: 44
                        height: 44
                        radius: 10
                        color: "#EEF1FD"
                        Layout.alignment: Qt.AlignVCenter

                        FluIcon {
                            anchors.centerIn: parent
                            iconSource: FluentIcons.NewFolder
                            iconSize: 24
                            color: "#4F6BED"
                        }
                    }

                    Column {
                        spacing: 3
                        Layout.alignment: Qt.AlignVCenter

                        FluText {
                            text: "新建文件夹"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#1a1a2e"
                        }

                        FluText {
                            text: "在当前目录下创建一个新文件夹"
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

                // ── 输入框 ──
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    spacing: 6

                    FluText {
                        text: "文件夹名称"
                        font.pixelSize: 13
                        color: "#444444"
                    }

                    FluTextBox {
                        id: nameInput
                        Layout.fillWidth: true
                        text: "新建文件夹"
                        placeholderText: "输入文件夹名称..."

                        // 回车直接确认
                        Keys.onReturnPressed: {
                            var name = nameInput.text.trim();
                            if (name.length > 0) {
                                FileController.createFolder(name);
                                root.close();
                            }
                        }
                    }
                }
            }
        }
    }
}
