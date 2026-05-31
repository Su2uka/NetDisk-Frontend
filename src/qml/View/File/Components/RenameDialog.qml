import QtQuick 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import App 1.0

/**
 * RenameDialog — 重命名弹窗
 *
 * 用法：RenameDialog { id: dlg }
 *       dlg.openDialog(fileInfo)
 */
FluContentDialog {
    id: root
    title: "重命名"
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "取消"
    positiveText: "重命名"

    property var _textBox: null
    property string _fileId: ""
    property string _oldName: ""

    function openDialog(info) {
        _fileId = info.fileId;
        _oldName = info.fileName;
        if (_textBox) {
            _textBox.text = _oldName;
            _textBox.forceActiveFocus();
            _textBox.selectAll();
        }
        root.open();
    }

    onPositiveClicked: {
        if (root._textBox) {
            var name = root._textBox.text.trim();
            if (name.length > 0 && name !== _oldName) {
                FileController.renameItem(_fileId, name);
            }
        }
    }

    onOpened: {
        if (root._textBox) {
            root._textBox.text = _oldName;
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

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.topMargin: 4
                    Layout.bottomMargin: 16
                    spacing: 14

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 10
                        color: "#EEF1FD"
                        Layout.alignment: Qt.AlignVCenter

                        FluIcon {
                            anchors.centerIn: parent
                            iconSource: FluentIcons.Rename
                            iconSize: 24
                            color: "#4F6BED"
                        }
                    }

                    Column {
                        spacing: 3
                        Layout.alignment: Qt.AlignVCenter

                        FluText {
                            text: "重命名项目"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: "#1a1a2e"
                        }

                        FluText {
                            text: "输入新的名称"
                            font.pixelSize: 12
                            color: "#888888"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.bottomMargin: 16
                    color: "#efefef"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    spacing: 6

                    FluText {
                        text: "名称"
                        font.pixelSize: 13
                        color: "#444444"
                    }

                    FluTextBox {
                        id: nameInput
                        Layout.fillWidth: true
                        placeholderText: "输入名称..."

                        Keys.onReturnPressed: {
                            var name = nameInput.text.trim();
                            if (name.length > 0 && name !== root._oldName) {
                                FileController.renameItem(root._fileId, name);
                                root.close();
                            } else {
                                root.close();
                            }
                        }
                    }
                }
            }
        }
    }
}
