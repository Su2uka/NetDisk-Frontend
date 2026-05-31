import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../File/Components"
import App 1.0

// ── 已完成列表：直接绑定 TransferHistory.model（持久化 C++ 模型） ──
Item {
    id: root

    // ── 内部只读属性 ──
    readonly property var _iconMap: ({
            "jpg": "ft-image",
            "jpeg": "ft-image",
            "png": "ft-image",
            "gif": "ft-image",
            "bmp": "ft-image",
            "svg": "ft-image",
            "webp": "ft-image",
            "ico": "ft-image",
            "tiff": "ft-image",
            "mp4": "ft-video",
            "avi": "ft-video",
            "mkv": "ft-video",
            "mov": "ft-video",
            "wmv": "ft-video",
            "flv": "ft-video",
            "webm": "ft-video",
            "m4v": "ft-video",
            "mp3": "ft-audio",
            "wav": "ft-audio",
            "flac": "ft-audio",
            "aac": "ft-audio",
            "ogg": "ft-audio",
            "wma": "ft-audio",
            "m4a": "ft-audio",
            "doc": "ft-word",
            "docx": "ft-word",
            "rtf": "ft-word",
            "xls": "ft-excel",
            "xlsx": "ft-excel",
            "csv": "ft-excel",
            "ppt": "ft-ppt",
            "pptx": "ft-ppt",
            "pdf": "ft-pdf",
            "txt": "ft-text",
            "md": "ft-text",
            "log": "ft-text",
            "js": "ft-code",
            "ts": "ft-code",
            "py": "ft-code",
            "java": "ft-code",
            "c": "ft-code",
            "cpp": "ft-code",
            "h": "ft-code",
            "hpp": "ft-code",
            "cs": "ft-code",
            "go": "ft-code",
            "rs": "ft-code",
            "rb": "ft-code",
            "php": "ft-code",
            "swift": "ft-code",
            "kt": "ft-code",
            "json": "ft-code",
            "xml": "ft-code",
            "yaml": "ft-code",
            "yml": "ft-code",
            "toml": "ft-code",
            "ini": "ft-code",
            "sh": "ft-code",
            "bat": "ft-code",
            "css": "ft-code",
            "html": "ft-web",
            "htm": "ft-web",
            "zip": "ft-archive",
            "rar": "ft-archive",
            "7z": "ft-archive",
            "tar": "ft-archive",
            "gz": "ft-archive",
            "bz2": "ft-archive",
            "xz": "ft-archive",
            "exe": "ft-exe",
            "msi": "ft-exe",
            "apk": "ft-apk",
            "dmg": "ft-apple-pkg",
            "pkg": "ft-apple-pkg",
            "iso": "ft-iso",
            "psd": "ft-psd",
            "dwg": "ft-cad",
            "dxf": "ft-cad"
        })

    // ── 函数 ──
    function formatBytes(bytes) {
        if (bytes <= 0)
            return "0 B";
        var units = ["B", "KB", "MB", "GB", "TB"];
        var i = Math.floor(Math.log(bytes) / Math.log(1024));
        if (i >= units.length)
            i = units.length - 1;
        return (bytes / Math.pow(1024, i)).toFixed(2) + " " + units[i];
    }

    function fileTypeIcon(fileName) {
        if (!fileName)
            return "qrc:/file_type/res/file_type/ft-unknown.svg";
        var ext = fileName.split('.').pop().toLowerCase();
        var icon = root._iconMap[ext] || "ft-unknown";
        return "qrc:/file_type/res/file_type/" + icon + ".svg";
    }

    function statusText(status, isUpload) {
        if (status === 3)
            return isUpload ? "已上传" : "已下载";
        if (status === 4)
            return "失败";
        return "已取消";
    }

    function statusColor(status) {
        if (status === 3)
            return "#16A34A";
        if (status === 4)
            return "#DC2626";
        return "#888888";
    }

    function confirmClearHistory() {
        dangerConfirmDialog.openDialog(
                    "清空传输记录",
                    "确定要清空全部 " + TransferHistory.model.count + " 条传输记录吗？此操作不会删除本地文件或网盘文件。",
                    "清空记录",
                    "clearHistory",
                    null);
    }

    // ── 子对象 ──
    DangerConfirmDialog {
        id: dangerConfirmDialog
        onConfirmed: function (action, payload) {
            if (action === "clearHistory")
                TransferHistory.clearAll();
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // 汇总栏
        Rectangle {
            width: parent.width
            height: 48
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                FluText {
                    text: "已完成 " + TransferHistory.model.count + " 项任务"
                    font.pixelSize: 13
                    color: "#444444"
                }

                Item {
                    Layout.fillWidth: true
                }

                FluIconButton {
                    display: Button.TextBesideIcon
                    text: "清空记录"
                    iconSource: FluentIcons.Delete
                    iconSize: 12
                    font.pixelSize: 12
                    visible: TransferHistory.model.count > 0
                    onClicked: root.confirmClearHistory()
                }
            }
        }

        // 表头
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
                    Layout.preferredWidth: 100
                    horizontalAlignment: Text.AlignHCenter
                }
                FluText {
                    text: "完成时间"
                    color: "#4F6BED"
                    font.pixelSize: 12
                    Layout.preferredWidth: 130
                }
                FluText {
                    text: "状态"
                    color: "#4F6BED"
                    font.pixelSize: 12
                    Layout.preferredWidth: 80
                }

                Item {
                    width: 32
                    height: 1
                }
            }
        }

        // 列表
        ListView {
            width: parent.width
            height: parent.height - 84
            clip: true
            model: TransferHistory.model
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 200

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Item {
                width: parent ? parent.width : 600
                height: 52

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    color: completedRowHover.hovered ? "#06000000" : "transparent"
                    radius: 6

                    HoverHandler {
                        id: completedRowHover
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#f0f0f0"
                        anchors.bottom: parent.bottom
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 16

                        Image {
                            source: root.fileTypeIcon(model.fileName)
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 28
                            sourceSize: Qt.size(28, 28)
                            fillMode: Image.PreserveAspectFit
                        }

                        FluText {
                            text: model.fileName
                            font.pixelSize: 13
                            color: "#222222"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        FluText {
                            text: root.formatBytes(model.fileSize)
                            font.pixelSize: 12
                            color: "#888888"
                            Layout.preferredWidth: 100
                            horizontalAlignment: Text.AlignHCenter
                        }

                        FluText {
                            text: model.finishedAt
                            font.pixelSize: 12
                            color: "#999999"
                            Layout.preferredWidth: 130
                        }

                        FluText {
                            text: root.statusText(model.status, model.isUpload)
                            font.pixelSize: 12
                            color: root.statusColor(model.status)
                            Layout.preferredWidth: 80
                        }

                        Item {
                            width: 32
                            height: 1
                        }
                    }
                }
            }

            FluText {
                anchors.centerIn: parent
                visible: TransferHistory.model.count === 0
                text: "暂无已完成任务"
                color: "#bbbbbb"
                font.pixelSize: 14
            }
        }
    }
}
