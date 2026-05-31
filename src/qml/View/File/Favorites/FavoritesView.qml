import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../Components"
import App 1.0

Item {
    id: root

    Component.onCompleted: Qt.callLater(FavoritesController.loadFavorites)

    Connections {
        target: FavoritesController
        function onFavOpSuccess(message) {
            showSuccess(message);
        }
        function onFavOpFailed(message) {
            showError(message);
        }
    }

    // ── 取消收藏确认弹窗 ──
    FluContentDialog {
        id: removeConfirmDialog
        title: "取消收藏"
        message: "确定要取消收藏所选项目吗？"
        buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
        negativeText: "再想想"
        positiveText: "确认"

        property string pendingFileId: ""
        property bool isBatch: false

        onPositiveClicked: {
            if (isBatch) {
                FavoritesController.removeSelectedFavorites();
            } else if (pendingFileId !== "") {
                FavoritesController.removeFavorite(pendingFileId);
            }
        }
    }

    // ── 右键菜单 ──
    FluMenu {
        id: contextMenu
        width: 180
        property int targetIndex: -1

        FluMenuItem {
            text: "前往文件位置"
            iconSource: FluentIcons.OpenInNewWindow
            onClicked: {
                var info = FavoritesController.fileModel.getFileInfo(contextMenu.targetIndex);
                if (info.isFolder) {
                    // 文件夹：直接跳转到该文件夹
                    FileController.navigateToFileLocation(info.fileId);
                } else {
                    // 文件：跳转到其父目录
                    FileController.navigateToFileLocation(info.parentId);
                }
            }
        }
        FluMenuSeparator {}
        FluMenuItem {
            text: "下载"
            iconSource: FluentIcons.Download
            onClicked: {
                var info = FavoritesController.fileModel.getFileInfo(contextMenu.targetIndex);
                downloadHelper.startDownload(info.fileId, info.fileName, info.fileSize, info.parentId || "");
            }
        }
        FluMenuItem {
            text: "查看详细信息"
            iconSource: FluentIcons.Info
            onClicked: {
                var info = FavoritesController.fileModel.getFileInfo(contextMenu.targetIndex);
                fileDetailDialog.showDetail(info);
            }
        }
        FluMenuSeparator {}
        FluMenuItem {
            text: "取消收藏"
            iconSource: FluentIcons.HeartBroken
            textColor: "#e74c3c"
            onClicked: {
                var info = FavoritesController.fileModel.getFileInfo(contextMenu.targetIndex);
                removeConfirmDialog.pendingFileId = info.fileId;
                removeConfirmDialog.isBatch = false;
                removeConfirmDialog.open();
            }
        }
    }

    FileDetailDialog {
        id: fileDetailDialog
    }

    FileDownloadHelper {
        id: downloadHelper
        rootWindow: window
    }

    // ── 主布局 ──
    Column {
        anchors.fill: parent
        spacing: 0

        // 工具栏
        FileToolBar {
            selectedCount: FavoritesController.selectedCount
            totalCount: FavoritesController.fileModel.count
            hasSelection: FavoritesController.hasSelection
            sortField: FavoritesController.sortField
            sortAsc: FavoritesController.sortAsc
            viewMode: FavoritesController.viewMode

            onSelectAll: FavoritesController.selectAll()
            onClearSelection: FavoritesController.clearSelection()
            onRequestSortField: function (field) {
                FavoritesController.sortField = field;
            }
            onRequestSortAsc: function (asc) {
                FavoritesController.sortAsc = asc;
            }
            onViewModeToggled: FavoritesController.viewMode = FavoritesController.viewMode === 0 ? 1 : 0
            onBatchAction: function (action) {
                if (action === "删除") {
                    removeConfirmDialog.isBatch = true;
                    removeConfirmDialog.pendingFileId = "";
                    removeConfirmDialog.open();
                } else {
                    console.log("批量操作:", action);
                }
            }
        }

        // 文件区域
        Item {
            width: parent.width
            height: parent.height - 84

            Loader {
                anchors.fill: parent
                active: FavoritesController.viewMode === 0
                sourceComponent: Component {
                    FileGridView {
                        anchors.fill: parent
                        model: FavoritesController.fileModel
                        hasSelection: FavoritesController.hasSelection
                        onToggleSelection: function (index) {
                            FavoritesController.toggleSelection(index);
                        }
                        onOpenFolder: function (id, name) {
                            // 双击文件夹 → 跳转到"我的文件"页面
                            FileController.navigateToFileLocation(id);
                        }
                        onContextMenuRequested: function (index) {
                            contextMenu.targetIndex = index;
                            contextMenu.popup();
                        }
                    }
                }
            }

            Loader {
                anchors.fill: parent
                active: FavoritesController.viewMode === 1
                sourceComponent: Component {
                    FileListView {
                        anchors.fill: parent
                        fileModel: FavoritesController.fileModel
                        hasSelection: FavoritesController.hasSelection
                        sortField: FavoritesController.sortField
                        sortAsc: FavoritesController.sortAsc
                        onToggleSelection: function (index) {
                            FavoritesController.toggleSelection(index);
                        }
                        onOpenFolder: function (id, name) {
                            // 双击文件夹 → 跳转到"我的文件"页面
                            FileController.navigateToFileLocation(id);
                        }
                        onOpenFile: function (id, parentId, name) {
                            console.log("打开文件:", name);
                        }
                        onContextMenuRequested: function (index) {
                            contextMenu.targetIndex = index;
                            contextMenu.popup();
                        }
                    }
                }
            }

            // 空状态
            FluText {
                anchors.centerIn: parent
                text: "暂无收藏文件"
                font.pixelSize: 14
                color: "#bbbbbb"
                visible: FavoritesController.fileModel.count === 0
            }

            // 空白区域右键
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                z: -1
                onClicked: function (mouse) {
                    if (mouse.button === Qt.RightButton)
                        blankMenu.popup();
                }
            }

            FluMenu {
                id: blankMenu
                width: 160
                FluMenuItem {
                    text: "刷新页面"
                    onClicked: FavoritesController.loadFavorites()
                }
            }
        }
    }
}
