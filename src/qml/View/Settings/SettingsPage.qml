import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import FluentUI
import App 1.0

Item {
    id: root

    // return to home signal
    signal settingsBackRequested()

    // top title bar
    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 52
        color: "#ffffff"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 16

            FluText {
                text: "设置"
                font.pixelSize: 20
                font.weight: Font.Medium
                color: "#222222"
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            FluIconButton {
                iconSource: FluentIcons.BackToWindow
                iconColor: "#555555"
                Layout.alignment: Qt.AlignVCenter
                onClicked: settingsBackRequested()
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#e5e5e5"
        }
    }

    Flickable {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        contentHeight: settingsCol.height + 40
        ScrollBar.vertical: FluScrollBar {}
        clip: true

        Column {
            id: settingsCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20
            spacing: 12

            // ── download settings card ──
            Rectangle {
                width: settingsCol.width
                height: downloadCardCol.height + 32
                color: "#ffffff"
                radius: 8

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    color: "#08000000"
                    radius: 8
                    z: -1
                }

                Column {
                    id: downloadCardCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 12

                    FluText {
                        text: "下载设置"
                        color: "#222222"
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        width: parent.width
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#f0f0f0"
                    }

                    RowLayout {
                        width: parent.width
                        height: 36

                        FluText {
                            text: "默认下载位置"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluTextBox {
                            id: dirTextBox
                            Layout.preferredWidth: 320
                            Layout.preferredHeight: 36
                            readOnly: true
                            text: SettingsController.downloadDir

                            Connections {
                                target: SettingsController
                                function onDownloadDirChanged() {
                                    dirTextBox.text = SettingsController.downloadDir;
                                }
                            }
                        }

                        FluTextButton {
                            text: "选择文件夹"
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: folderDialog.open()
                        }
                    }
                }
            }

            // ── transfer settings card ──
            Rectangle {
                width: settingsCol.width
                height: transferCardCol.height + 32
                color: "#ffffff"
                radius: 8

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    color: "#08000000"
                    radius: 8
                    z: -1
                }

                Column {
                    id: transferCardCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 12

                    FluText {
                        text: "传输设置"
                        color: "#222222"
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        width: parent.width
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#f0f0f0"
                    }

                    // upload concurrent
                    RowLayout {
                        width: parent.width
                        height: 44

                        FluText {
                            text: "上传并发数"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluComboBox {
                            id: uploadCombo
                            Layout.preferredWidth: 100
                            Layout.alignment: Qt.AlignVCenter
                            model: ["1", "2", "3"]
                            currentIndex: SettingsController.maxUploadConcurrent - 1

                            Connections {
                                target: SettingsController
                                function onMaxUploadConcurrentChanged() {
                                    uploadCombo.currentIndex = SettingsController.maxUploadConcurrent - 1;
                                }
                            }
                            onActivated: SettingsController.maxUploadConcurrent = currentIndex + 1
                        }

                        FluText {
                            text: "个任务并发上传"
                            color: "#888888"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    // download concurrent
                    RowLayout {
                        width: parent.width
                        height: 44

                        FluText {
                            text: "下载并发数"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluComboBox {
                            id: downloadCombo
                            Layout.preferredWidth: 100
                            Layout.alignment: Qt.AlignVCenter
                            model: ["1", "2", "3"]
                            currentIndex: SettingsController.maxDownloadConcurrent - 1

                            Connections {
                                target: SettingsController
                                function onMaxDownloadConcurrentChanged() {
                                    downloadCombo.currentIndex = SettingsController.maxDownloadConcurrent - 1;
                                }
                            }
                            onActivated: SettingsController.maxDownloadConcurrent = currentIndex + 1
                        }

                        FluText {
                            text: "个任务并发下载"
                            color: "#888888"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    // auto start upload
                    RowLayout {
                        width: parent.width
                        height: 44

                        FluText {
                            text: "自动开始上传"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluToggleSwitch {
                            id: uploadAutoSwitch
                            checked: SettingsController.autoStartUpload

                            Connections {
                                target: SettingsController
                                function onAutoStartUploadChanged() {
                                    uploadAutoSwitch.checked = SettingsController.autoStartUpload;
                                }
                            }
                            onClicked: SettingsController.autoStartUpload = checked
                        }

                        FluText {
                            text: "添加上传任务后自动开始传输"
                            color: "#888888"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    // auto start download
                    RowLayout {
                        width: parent.width
                        height: 44

                        FluText {
                            text: "自动开始下载"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluToggleSwitch {
                            id: downloadAutoSwitch
                            checked: SettingsController.autoStartDownload

                            Connections {
                                target: SettingsController
                                function onAutoStartDownloadChanged() {
                                    downloadAutoSwitch.checked = SettingsController.autoStartDownload;
                                }
                            }
                            onClicked: SettingsController.autoStartDownload = checked
                        }

                        FluText {
                            text: "添加下载任务后自动开始传输"
                            color: "#888888"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }

            // ── about card ──
            Rectangle {
                width: settingsCol.width
                height: aboutCardCol.height + 32
                color: "#ffffff"
                radius: 8

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    color: "#08000000"
                    radius: 8
                    z: -1
                }

                Column {
                    id: aboutCardCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 12

                    FluText {
                        text: "关于"
                        color: "#222222"
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        width: parent.width
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#f0f0f0"
                    }

                    RowLayout {
                        width: parent.width
                        height: 36

                        FluText {
                            text: "版本"
                            color: "#333333"
                            font.pixelSize: 14
                            Layout.preferredWidth: 120
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        FluText {
                            text: "1.0.0"
                            color: "#888888"
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }
        }
    }

    // ── FolderDialog placed at root level ──
    FolderDialog {
        id: folderDialog
        title: "选择默认下载位置"
        onAccepted: {
            SettingsController.downloadDir = selectedFolder;
        }
    }
}