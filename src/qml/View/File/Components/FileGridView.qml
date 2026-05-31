import QtQuick 2.15
import QtQuick.Controls 2.15
import FluentUI 1.0
import App 1.0

GridView {
    id: root
    clip: true
    cellWidth: 100
    cellHeight: 120
    leftMargin: 20
    topMargin: 1
    boundsBehavior: Flickable.StopAtBounds

    // ── 属性 ──
    property bool hasSelection: false

    // ── 信号 ──
    signal toggleSelection(int index)
    signal openFolder(string id, string name)
    signal openFile(string id, string parentId, string name)
    signal contextMenuRequested(int index)

    // ── 无限滚动 ──
    onContentYChanged: {
        if (!FileController.hasMore || FileController.loadingMore)
            return;
        // 当滚动到距离底部 120px 以内时，触发加载更多
        if (contentHeight > height && contentY + height > contentHeight - 120) {
            FileController.loadMoreFiles();
        }
    }

    // ── 底部加载指示器 ──
    footer: Item {
        width: root.width
        height: FileController.loadingMore ? 48 : 0
        visible: FileController.loadingMore

        BusyIndicator {
            anchors.centerIn: parent
            running: FileController.loadingMore
            width: 24
            height: 24
        }
    }

    // ── 子对象 ──
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AlwaysOff
    }

    delegate: Item {
        width: root.cellWidth
        height: root.cellHeight

        Rectangle {
            id: cellBg
            anchors.fill: parent
            anchors.margins: 4
            radius: 8
            color: model.selected ? "#15000000" : (cellHover.hovered ? "#08000000" : "transparent")

            Behavior on color {
                ColorAnimation {
                    duration: 120
                }
            }
        }

        HoverHandler {
            id: cellHover
            cursorShape: Qt.PointingHandCursor
        }

        // 右键菜单
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: function (mouse) {
                root.contextMenuRequested(index);
            }
        }

        TapHandler {
            onDoubleTapped: {
                if (model.isFolder)
                    root.openFolder(model.fileId, model.fileName);
                else
                    root.openFile(model.fileId, model.parentId || "", model.fileName);
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 6

            // ── 图标/缩略图容器 ──
            Item {
                width: 56
                height: 56
                anchors.horizontalCenter: parent.horizontalCenter

                // 缩略图（仅图片文件有 thumbnailUrl）
                Rectangle {
                    id: thumbnailContainer
                    anchors.fill: parent
                    radius: 6
                    clip: true
                    color: "transparent"
                    visible: thumbnail.status === Image.Ready

                    Image {
                        id: thumbnail
                        anchors.fill: parent
                        source: model.thumbnailUrl || ""
                        fillMode: Image.PreserveAspectCrop
                        sourceSize: Qt.size(120, 120)
                        asynchronous: true
                        cache: false
                    }
                }

                // 默认文件类型图标（缩略图不可用时显示）
                Image {
                    anchors.fill: parent
                    source: model.fileIcon
                    visible: !thumbnailContainer.visible
                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(56, 56)
                }
            }

            FluText {
                text: model.fileName
                font.pixelSize: 12
                color: "#333333"
                width: root.cellWidth - 24
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideMiddle
                maximumLineCount: 2
                wrapMode: Text.WrapAnywhere
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // 选中复选框
        FluCheckBox {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 6
            anchors.leftMargin: 6
            visible: root.hasSelection || cellHover.hovered
            checked: model.selected
            size: 18
            clickListener: function () {
                root.toggleSelection(index);
            }
        }
    }
}

