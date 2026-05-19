import QtQuick 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

FluContentDialog {
    id: control

    property var fileInfo: ({})

    title: "文件详情"
    buttonFlags: FluContentDialogType.NeutralButton
    neutralText: "关闭"

    contentDelegate: Component {
        Item {
            implicitHeight: detailLayout.height + 32

            ColumnLayout {
                id: detailLayout
                width: parent.width
                spacing: 0

                // 图标 + 文件名
                RowLayout {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 8
                    spacing: 12

                    Image {
                        source: control.fileInfo.fileIcon || ""
                        sourceSize: Qt.size(40, 40)
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                    }

                    FluText {
                        text: control.fileInfo.fileName || ""
                        font.pixelSize: 15
                        font.bold: true
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }

                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: 12
                    Layout.bottomMargin: 4
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    color: "#e8e8e8"
                }

                // 详情行
                Repeater {
                    model: [
                        {
                            label: "类型",
                            value: control.fileInfo.isFolder ? "文件夹" : "文件"
                        },
                        {
                            label: "创建时间",
                            value: control.fileInfo.createTime || "—"
                        },
                        {
                            label: "修改时间",
                            value: control.fileInfo.modifyTime || "—"
                        },
                        {
                            label: "大小",
                            value: control.fileInfo.isFolder ? "—" : (control.fileInfo.fileSizeStr || "—")
                        }
                    ]

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.topMargin: 8

                        FluText {
                            text: modelData.label
                            font.pixelSize: 13
                            color: "#888888"
                            Layout.preferredWidth: 70
                        }

                        FluText {
                            text: modelData.value
                            font.pixelSize: 13
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }
                }

                // 底部间距
                Item {
                    Layout.preferredHeight: 8
                }
            }
        }
    }

    function showDetail(info) {
        control.fileInfo = info;
        control.open();
    }
}
