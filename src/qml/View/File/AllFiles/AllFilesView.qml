import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../Components"
import App 1.0

Item {
    id: root

    property var uploadHelper: null
    property var createFolderDialog: null

    function batchDownloadSelected() {
        var items = [];
        var count = FileController.fileModel.count;
        for (var i = 0; i < count; i++) {
            var info = FileController.fileModel.getFileInfo(i);
            if (!info.isSelected)
                continue;
            items.push({
                fileId: info.fileId,
                fileName: info.fileName,
                fileSize: info.fileSize,
                isFolder: info.isFolder
            });
        }
        if (items.length > 0)
            downloadHelper.startBatchDownload(items);
    }

    Component.onCompleted: Qt.callLater(FileController.loadFiles)


    Connections {
        target: FileController
        function onTrashSuccess(message) {
            showSuccess(message);
        }
    }

    Connections {
        target: ShareController
        function onShowToast(message, isError) {
            if (isError) {
                showError(message);
            } else {
                showSuccess(message);
            }
        }
    }

    Connections {
        target: FavoritesController
        function onFavOpSuccess(message) {
            showSuccess(message);
        }
        function onFavOpFailed(message) {
            showError(message);
        }
    }

    FileContextMenu {
        id: contextMenu
        onActionTriggered: function (action, index) {
            var info = FileController.fileModel.getFileInfo(index);
            if (action === "detail") {
                fileDetailDialog.showDetail(info);
            } else if (action === "share") {
                shareDialog.openDialog(info);
            } else if (action === "download") {
                if (FileController.selectedCount > 1) {
                    root.batchDownloadSelected();
                } else if (info.isFolder) {
                    downloadHelper.startFolderDownload(info.fileId, info.fileName);
                } else {
                    downloadHelper.startDownload(info.fileId, info.fileName, info.fileSize);
                }
            } else if (action === "favorite") {
                FavoritesController.addFavorite(info.fileId);
            } else if (action === "delete") {
                if (FileController.selectedCount > 1)
                    FileController.moveSelectedToTrash();
                else
                    FileController.moveToTrash(info.fileId);
            } else {
                console.log(action + ":", index);
            }
        }
    }

    FileDetailDialog {
        id: fileDetailDialog
    }

    ShareDialog {
        id: shareDialog
    }

    FileDownloadHelper {
        id: downloadHelper
    }

    // 布局
    Column {
        anchors.fill: parent
        spacing: 0

        PathBar {
            pathModel: FileController.breadcrumbs
            onNavigateTo: function (index) {
                FileController.navigateTo(index);
            }
        }

        FileToolBar {
            selectedCount: FileController.selectedCount
            totalCount: FileController.fileModel.count
            hasSelection: FileController.hasSelection
            sortField: FileController.sortField
            sortAsc: FileController.sortAsc
            viewMode: FileController.viewMode

            onSelectAll: FileController.selectAll()
            onClearSelection: FileController.clearSelection()
            onRequestSortField: function (field) {
                FileController.sortField = field;
            }
            onRequestSortAsc: function (asc) {
                FileController.sortAsc = asc;
            }
            onViewModeToggled: FileController.viewMode = FileController.viewMode === 0 ? 1 : 0
            onBatchAction: function (action) {
                if (action === "下载")
                    root.batchDownloadSelected();
                else if (action === "删除")
                    FileController.moveSelectedToTrash();
                else
                    console.log("批量操作:", action);
            }
        }

        // 文件区域
        Item {
            width: parent.width
            height: parent.height - 84

            Loader {
                anchors.fill: parent
                active: FileController.viewMode === 0
                sourceComponent: Component {
                    FileGridView {
                        anchors.fill: parent
                        model: FileController.fileModel
                        hasSelection: FileController.hasSelection
                        onToggleSelection: function (index) {
                            FileController.toggleSelection(index);
                        }
                        onOpenFolder: function (id, name) {
                            FileController.enterFolder(id, name);
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
                active: FileController.viewMode === 1
                sourceComponent: Component {
                    FileListView {
                        anchors.fill: parent
                        fileModel: FileController.fileModel
                        hasSelection: FileController.hasSelection
                        sortField: FileController.sortField
                        sortAsc: FileController.sortAsc
                        onToggleSelection: function (index) {
                            FileController.toggleSelection(index);
                        }
                        onOpenFolder: function (id, name) {
                            FileController.enterFolder(id, name);
                        }
                        onOpenFile: function (name) {
                            console.log("打开文件:", name);
                        }
                        onContextMenuRequested: function (index) {
                            contextMenu.targetIndex = index;
                            contextMenu.popup();
                        }
                    }
                }
            }

            // 空白区域右键菜单
            FluMenu {
                id: blankAreaMenu
                width: 160

                FluMenuItem {
                    text: "新建文件夹"
                    onClicked: {
                        if (root.createFolderDialog)
                            root.createFolderDialog.open();
                    }
                }
                FluMenuItem {
                    text: "上传文件"
                    onClicked: {
                        if (root.uploadHelper)
                            root.uploadHelper.openFileDialog();
                    }
                }
                FluMenuItem {
                    text: "上传文件夹"
                    onClicked: {
                        if (root.uploadHelper)
                            root.uploadHelper.openFolderDialog();
                    }
                }
                FluMenuItem {
                    text: "刷新页面"
                    onClicked: FileController.loadFiles()
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                z: -1
                onClicked: function (mouse) {
                    if (mouse.button === Qt.RightButton)
                        blankAreaMenu.popup();
                }
            }
        }
    }
}
