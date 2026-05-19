import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import App 1.0

// ── 下载列表子视图 ──
Item {
    id: root

    Column {
        anchors.fill: parent
        spacing: 0

        // ── 汇总栏 ──
        Rectangle {
            width: parent.width
            height: 48
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                FluText {
                    text: "下载列表 · " + DownloadController.model.count + " 项任务"
                    font.pixelSize: 13
                    color: "#444444"
                }

                Item {
                    Layout.fillWidth: true
                }

                Row {
                    spacing: 12
                    Layout.alignment: Qt.AlignVCenter
                    visible: DownloadController.model.count > 0

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部开始"
                        iconSource: FluentIcons.Play
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: DownloadController.resumeAll()
                    }

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部暂停"
                        iconSource: FluentIcons.Pause
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: DownloadController.pauseAll()
                    }

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部取消"
                        iconSource: FluentIcons.Cancel
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: DownloadController.cancelAll()
                    }
                }
            }
        }

        // ── 表头 ──
        Rectangle {
            width: parent.width
            height: 36
            color: "transparent"

            Rectangle {
                width: parent.width - 40
                height: 1
                color: "#f0f0f0"
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 32
                anchors.rightMargin: 32
                spacing: 16

                Item {
                    width: 28
                    height: 1
                }

                FluText {
                    text: "名称"
                    color: "#4F6BED"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                FluText {
                    text: "大小"
                    color: "#4F6BED"
                    font.pixelSize: 12
                    Layout.preferredWidth: 140
                    horizontalAlignment: Text.AlignLeft
                }

                FluText {
                    text: "状态"
                    color: "#4F6BED"
                    font.pixelSize: 12
                    Layout.preferredWidth: 160
                }

                Item {
                    width: 64
                    height: 1
                }
            }
        }

        // ── 任务列表 ──
        ListView {
            width: parent.width
            height: parent.height - 84
            clip: true
            model: DownloadController.model
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 200

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: TransferTaskRow {
                visible: model.status < 3
                height: model.status < 3 ? 56 : 0
                taskId: model.taskId
                fileName: model.fileName
                totalBytes: model.totalBytes
                receivedBytes: model.receivedBytes
                progress: model.progress
                taskStatus: {
                    switch (model.status) {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return 2;
                    default:
                        return 0;
                    }
                }
                errorMsg: model.errorMsg
                isUpload: false
                onRequestPause: function (tid) {
                    DownloadController.pauseDownload(tid);
                }
                onRequestResume: function (tid) {
                    DownloadController.startDownload(tid);
                }
                onRequestCancel: function (tid) {
                    DownloadController.cancelDownload(tid);
                }
            }

            // 空状态
            FluText {
                anchors.centerIn: parent
                visible: DownloadController.model.count === 0
                text: "暂无下载任务"
                color: "#bbbbbb"
                font.pixelSize: 14
            }
        }
    }
}
