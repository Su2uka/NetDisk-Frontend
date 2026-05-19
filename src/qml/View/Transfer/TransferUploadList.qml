import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import App 1.0

// ── 上传列表子视图 ──
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
                    text: "上传列表 · " + UploadController.model.count + " 项任务"
                    font.pixelSize: 13
                    color: "#444444"
                }

                Item {
                    Layout.fillWidth: true
                }

                Row {
                    spacing: 12
                    Layout.alignment: Qt.AlignVCenter
                    visible: UploadController.model.count > 0

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部开始"
                        iconSource: FluentIcons.Play
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: UploadController.resumeAll()
                    }

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部暂停"
                        iconSource: FluentIcons.Pause
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: UploadController.pauseAll()
                    }

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "全部取消"
                        iconSource: FluentIcons.Cancel
                        iconSize: 12
                        font.pixelSize: 12
                        onClicked: UploadController.cancelAll()
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
            model: UploadController.model
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 200

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: TransferTaskRow {
                // status: 0=Queued, 1=Hashing, 2=Uploading, 3=Paused, 4=Success, 5=Failed, 6=Canceled
                visible: model.status < 4
                height: model.status < 4 ? 56 : 0
                taskId: model.taskId
                fileName: model.fileName
                totalBytes: model.fileSize
                receivedBytes: model.receivedBytes
                progress: model.progress
                taskStatus: model.status === 2 ? 1 : (model.status === 3 ? 2 : 0)
                errorMsg: model.errorMsg
                isUpload: true
                onRequestPause: function (tid) {
                    UploadController.pauseTask(tid);
                }
                onRequestResume: function (tid) {
                    UploadController.resumeTask(tid);
                }
                onRequestCancel: function (tid) {
                    UploadController.cancelTask(tid);
                }
            }

            // 空状态
            FluText {
                anchors.centerIn: parent
                visible: UploadController.model.count === 0
                text: "暂无上传任务"
                color: "#bbbbbb"
                font.pixelSize: 14
            }
        }
    }
}
