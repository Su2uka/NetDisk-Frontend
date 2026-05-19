import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../../Common"
import "Components"
import App 1.0

Item {
    id: root

    // ── 属性 ──
    property int currentIndex: 0

    // ── 子对象 ──

    // 上传助手（全局唯一实例，FAB 和右键菜单共用）
    FileUploadHelper {
        id: uploadHelper
        onUploadStarted: function (fileCount) {
            console.log("已加入上传队列:", fileCount, "个文件");
        }
    }

    // 新建文件夹弹窗
    CreateFolderDialog {
        id: createFolderDialog
    }

    // 监听文件夹创建成功信号
    Connections {
        target: FileController
        function onFolderCreated(message) {
            showSuccess(message);
        }
        function onGoToFileLocationRequested(folderId) {
            if (root.currentIndex !== 0) {
                root.currentIndex = 0;
                navBar.currentIndex = 0;
            }
        }
    }

    // 顶部 IconPivot 导航栏
    IconPivot {
        id: navBar
        width: parent.width
        currentIndex: root.currentIndex
        tabModels: [
            {
                title: "全部文件",
                icon: FluentIcons.FolderHorizontal
            },
            {
                title: "我的分享",
                icon: FluentIcons.Share
            },
            {
                title: "我的收藏",
                icon: FluentIcons.FavoriteStar
            },
            {
                title: "回收站",
                icon: FluentIcons.Delete
            }
        ]
        onCurrentIndexChanged: {
            root.currentIndex = currentIndex;
        }
    }

    // 页面内容区域（Loader 懒加载）
    Item {
        id: pageContainer
        anchors.top: navBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        readonly property var pageUrls: ["AllFiles/AllFilesView.qml", "../File/MyShares/MySharesView.qml", "../File/Favorites/FavoritesView.qml", "../File/RecycleBin/RecycleBinView.qml"]

        Loader {
            id: contentLoader
            anchors.fill: parent
            source: pageContainer.pageUrls[root.currentIndex]

            onLoaded: {
                if (root.currentIndex === 0 && item) {
                    if (item.uploadHelper !== undefined)
                        item.uploadHelper = uploadHelper;
                    if (item.createFolderDialog !== undefined)
                        item.createFolderDialog = createFolderDialog;
                }
            }
        }
    }

    // 右下角悬浮加号按钮 (FAB)
    RoundButton {
        id: fab
        width: 54
        height: 54
        radius: 27
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: 24
        z: 10
        visible: root.currentIndex === 0

        property bool menuWasOpen: false

        icon.source: ""
        flat: true

        background: Rectangle {
            radius: fab.radius
            color: fab.pressed ? "#3949AB" : (fab.hovered ? "#4558d0" : "#4F6BED")

            Behavior on color {
                ColorAnimation {
                    duration: 120
                }
            }
        }

        contentItem: FluIcon {
            anchors.centerIn: parent
            iconSource: FluentIcons.Add
            iconSize: 22
            color: "#ffffff"
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        onPressed: {
            fab.menuWasOpen = fabMenu.visible;
        }

        onClicked: {
            if (fab.menuWasOpen)
                fabMenu.close();
            else
                fabMenu.open();
        }

        FluMenu {
            id: fabMenu
            width: 160
            x: fab.width - width
            y: -height - 8

            FluMenuItem {
                text: "新建文件夹"
                iconSource: FluentIcons.NewFolder
                onClicked: {
                    fabMenu.close();
                    createFolderDialog.open();
                }
            }
            FluMenuItem {
                text: "上传文件"
                iconSource: FluentIcons.OpenFile
                onClicked: {
                    fabMenu.close();
                    uploadHelper.openFileDialog();
                }
            }
            FluMenuItem {
                text: "上传文件夹"
                iconSource: FluentIcons.MoveToFolder
                onClicked: {
                    fabMenu.close();
                    uploadHelper.openFolderDialog();
                }
            }
        }
    }
}
