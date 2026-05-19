import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

// ── 单个传输任务行（上传/下载共用） ──
Item {
    id: root
    width: parent ? parent.width : 600
    height: 56

    // ── 属性（数据输入） ──
    property string taskId: ""
    property string fileName: ""
    property real totalBytes: 0
    property real receivedBytes: 0
    property int progress: 0       // 0-100
    property int taskStatus: 0     // 0=Queued, 1=Active, 2=Paused
    property string errorMsg: ""
    property bool isUpload: true   // true=上传, false=下载

    // ── 信号 ──
    signal requestPause(string taskId)
    signal requestResume(string taskId)
    signal requestCancel(string taskId)

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
            "xls": "ft-excel",
            "xlsx": "ft-excel",
            "ppt": "ft-ppt",
            "pptx": "ft-ppt",
            "pdf": "ft-pdf",
            "txt": "ft-text",
            "md": "ft-text",
            "log": "ft-text",
            "zip": "ft-archive",
            "rar": "ft-archive",
            "7z": "ft-archive",
            "exe": "ft-exe",
            "msi": "ft-exe",
            "apk": "ft-apk",
            "psd": "ft-psd",
            "iso": "ft-iso"
        })

    // ── 函数 ──
    function fileTypeIcon(name) {
        if (!name)
            return "qrc:/file_type/res/file_type/ft-unknown.svg";
        var ext = name.split('.').pop().toLowerCase();
        var icon = root._iconMap[ext] || "ft-unknown";
        return "qrc:/file_type/res/file_type/" + icon + ".svg";
    }

    function formatBytes(bytes) {
        if (bytes <= 0)
            return "0 B";
        var units = ["B", "KB", "MB", "GB", "TB"];
        var i = Math.floor(Math.log(bytes) / Math.log(1024));
        if (i >= units.length)
            i = units.length - 1;
        return (bytes / Math.pow(1024, i)).toFixed(1) + " " + units[i];
    }

    function statusText() {
        switch (root.taskStatus) {
        case 0:
            return "等待中";
        case 1:
            return root.progress + "%";
        case 2:
            return "已暂停";
        default:
            return "";
        }
    }

    function statusColor() {
        switch (root.taskStatus) {
        case 0:
            return "#888888";
        case 1:
            return "#4F6BED";
        case 2:
            return "#F0A30A";
        default:
            return "#888888";
        }
    }

    function sizeDisplayText() {
        if (root.taskStatus === 1 || root.taskStatus === 2)
            return root.formatBytes(root.receivedBytes) + " / " + root.formatBytes(root.totalBytes);
        return root.formatBytes(root.totalBytes);
    }

    // ── 子对象 ──
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        color: rowHover.hovered ? "#06000000" : "transparent"
        radius: 6

        HoverHandler {
            id: rowHover
        }

        // 底部分割线
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

            // 文件图标
            Image {
                source: root.fileTypeIcon(root.fileName)
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                sourceSize: Qt.size(28, 28)
                fillMode: Image.PreserveAspectFit
            }

            // 文件名
            FluText {
                text: root.fileName
                font.pixelSize: 13
                color: "#222222"
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            // 大小
            FluText {
                text: root.sizeDisplayText()
                font.pixelSize: 12
                color: "#888888"
                Layout.preferredWidth: 140
                horizontalAlignment: Text.AlignLeft
            }

            // 状态 + 进度条列
            Column {
                Layout.preferredWidth: 160
                spacing: 4
                Layout.alignment: Qt.AlignVCenter

                FluProgressBar {
                    width: parent.width
                    visible: root.taskStatus === 1 || root.taskStatus === 2
                    indeterminate: false
                    value: root.progress / 100.0
                    height: 4
                    strokeWidth: 4
                }

                FluText {
                    text: root.statusText()
                    font.pixelSize: 12
                    color: root.statusColor()
                }
            }

            // 操作按钮（hover 时才显示）
            Row {
                spacing: 4
                Layout.alignment: Qt.AlignVCenter
                visible: rowHover.hovered && root.taskStatus <= 2

                FluIconButton {
                    visible: root.taskStatus === 1 || root.taskStatus === 2
                    iconSource: root.taskStatus === 2 ? FluentIcons.Play : FluentIcons.Pause
                    iconSize: 14
                    width: 28
                    height: 28
                    onClicked: {
                        if (root.taskStatus === 2)
                            root.requestResume(root.taskId);
                        else
                            root.requestPause(root.taskId);
                    }
                }

                FluIconButton {
                    iconSource: FluentIcons.Cancel
                    iconSize: 14
                    width: 28
                    height: 28
                    onClicked: root.requestCancel(root.taskId)
                }
            }

            // 占位（保持列宽一致，当按钮不可见时）
            Item {
                width: 64
                height: 1
                visible: !rowHover.hovered || root.taskStatus > 2
            }
        }
    }
}
