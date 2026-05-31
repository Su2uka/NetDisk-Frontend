import QtQuick
import QtQuick.Controls
import QtQuick.Window
import FluentUI 1.0
import MediaPlayer as DemoMediaPlayer

Item {
    id: control

    // 持久化的播放器窗口引用（生命周期 = 主程序存活期间）
    property var playerWindow: null
    // 图片窗口仍然独立管理
    property var openedWindows: []

    visible: false

    Component {
        id: mediaPlayerWindowComponent

        DemoMediaPlayer.Main {
        }
    }

    Component {
        id: imagePreviewWindowComponent

        FluWindow {
            id: imageWindow

            property url source
            property string fileName: ""

            width: 1200 * 0.8
            height: 780 * 0.8
            minimumWidth: 640
            minimumHeight: 460
            visible: true
            backgroundColor: "#09102B"
            title: fileName || qsTr("Image Preview")
            windowIcon: ""
            showDark: false

            appBar: FluAppBar {
                title: imageWindow.title
                height: 30
                showDark: false
                showClose: imageWindow.showClose
                showMinimize: imageWindow.showMinimize
                showMaximize: imageWindow.showMaximize
                showStayTop: imageWindow.showStayTop
                icon: imageWindow.windowIcon
                textColor: "#FFFFFF"
            }

            Image {
                id: imagePreview
                anchors.fill: parent
                anchors.margins: 28
                source: imageWindow.source
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: false
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: imagePreview.status === Image.Loading
                visible: running
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 64
                color: "#09102B"
                opacity: 0.82
            }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.bottomMargin: 20
                text: imageWindow.fileName
                color: "#ffffff"
                opacity: 0.86
                font.pixelSize: 14
                elide: Text.ElideMiddle
            }

            Shortcut {
                sequences: [StandardKey.Close]
                onActivated: imageWindow.close()
            }
        }
    }

    function openPreview(url, type, mime, name) {
        if (type === "image") {
            openImageWindow(url, name);
            return;
        }

        openMediaWindow(url, type, name);
    }

    function openMediaWindow(url, type, name) {
        if (playerWindow) {
            // 窗口已存在：直接添加文件并播放，复用窗口
            playerWindow.title = name || qsTr("Media Preview");
            playerWindow.openFile(url, name || "");
            playerWindow.show();
            playerWindow.raise();
            playerWindow.requestActivate();
            return;
        }

        // 首次创建
        playerWindow = mediaPlayerWindowComponent.createObject(null, {
            "source": url,
            "sourceName": name || "",
            "nameFilters": nameFiltersFor(type),
            "selectedNameFilter": type === "audio" ? 1 : 0
        });

        if (!playerWindow) {
            console.warn("[FilePreviewDialog] Failed to create media preview window");
            return;
        }

        playerWindow.title = name || qsTr("Media Preview");
        playerWindow.show();
        playerWindow.raise();
        playerWindow.requestActivate();
    }

    function openImageWindow(url, name) {
        var imageWindow = imagePreviewWindowComponent.createObject(null, {
            "source": url,
            "fileName": name || ""
        });

        if (!imageWindow) {
            console.warn("[FilePreviewDialog] Failed to create image preview window");
            return;
        }

        trackWindow(imageWindow);
    }

    function trackWindow(windowObject) {
        // 使用赋值而非 push()，确保触发 QML 属性变更通知，防止 GC 回收窗口
        openedWindows = openedWindows.concat([windowObject]);
        windowObject.closing.connect(function () {
            var closingWindow = windowObject;
            Qt.callLater(function () {
                untrackWindow(closingWindow);
                // FluWindow (autoDestroy: true) 会自行销毁，无需手动 destroy()
            });
        });
    }

    function untrackWindow(windowObject) {
        var nextWindows = [];
        for (var i = 0; i < openedWindows.length; ++i) {
            if (openedWindows[i] !== windowObject)
                nextWindows.push(openedWindows[i]);
        }
        openedWindows = nextWindows;
    }

    function nameFiltersFor(type) {
        if (type === "audio") {
            return [
                qsTr("Audio files (*.mp3 *.wav *.aac *.m4a *.flac *.ogg)"),
                qsTr("Video files (*.mp4 *.webm *.mov *.m4v)"),
                qsTr("All files (*)")
            ];
        }

        return [
            qsTr("Video files (*.mp4 *.webm *.mov *.m4v)"),
            qsTr("Audio files (*.mp3 *.wav *.aac *.m4a *.flac *.ogg)"),
            qsTr("All files (*)")
        ];
    }
}
