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
                parentId: info.parentId || "",
                fileName: info.fileName,
                fileSize: info.fileSize,
                isFolder: info.isFolder
            });
        }
        if (items.length > 0) {
            downloadHelperLoader.active = true;
            downloadHelperLoader.item.startBatchDownload(items);
        }
    }

    function previewInfo(info) {
        if (!info || info.isFolder) {
            showError("仅支持预览图片、视频和音频文件");
            return;
        }
        PreviewController.previewFile(info.fileId, info.parentId || "", info.fileName, info.isFolder);
    }

    function confirmMoveToTrash(info, isBatch) {
        var selectedCount = FileController.selectedCount;
        if (isBatch) {
            dangerConfirmDialog.openDialog(
                        "放入回收站",
                        "确定要将选中的 " + selectedCount + " 个项目放入回收站吗？",
                        "放入回收站",
                        "trashSelected",
                        null);
            return;
        }

        dangerConfirmDialog.openDialog(
                    "放入回收站",
                    "确定要将「" + info.fileName + "」放入回收站吗？",
                    "放入回收站",
                    "trashOne",
                    info);
    }

    Component.onCompleted: {
        if (!FileController.searchMode)
            Qt.callLater(FileController.loadFiles);
    }


    // ── 全局信号统一监听（合并） ──
    Connections {
        target: FileController
        function onRenameSuccess() {
            showSuccess("重命名成功");
        }
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
    Connections {
        target: PreviewController
        function onPreviewReady(previewUrl, mediaType, mimeType, fileName) {
            previewDialogLoader.active = true;
            previewDialogLoader.item.openPreview(previewUrl, mediaType, mimeType, fileName);
        }
        function onPreviewFailed(message) {
            showError(message);
        }
    }

    FileContextMenu {
        id: contextMenu
        onActionTriggered: function (action, index) {
            var info = FileController.fileModel.getFileInfo(index);
            if (action === "preview") {
                root.previewInfo(info);
            } else if (action === "detail") {
                detailDialogLoader.active = true;
                detailDialogLoader.item.showDetail(info);
            } else if (action === "share") {
                shareDialogLoader.active = true;
                shareDialogLoader.item.openDialog(info);
            } else if (action === "download") {
                downloadHelperLoader.active = true;
                if (FileController.selectedCount > 1) {
                    root.batchDownloadSelected();
                } else if (info.isFolder) {
                    downloadHelperLoader.item.startFolderDownload(info.fileId, info.fileName);
                } else {
                    downloadHelperLoader.item.startDownload(info.fileId, info.fileName, info.fileSize, info.parentId || "");
                }
            } else if (action === "rename") {
                renameDialogLoader.active = true;
                renameDialogLoader.item.openDialog(info);
            } else if (action === "favorite") {
                FavoritesController.addFavorite(info.fileId);
            } else if (action === "delete") {
                if (FileController.selectedCount > 1)
                    root.confirmMoveToTrash(info, true);
                else
                    root.confirmMoveToTrash(info, false);
            } else {
                console.log(action + ":", index);
            }
        }
    }

    // ── 懒加载弹窗组件（首次使用时才创建） ──

    Loader {
        id: renameDialogLoader
        active: false
        sourceComponent: Component { RenameDialog {} }
    }

    Loader {
        id: detailDialogLoader
        active: false
        sourceComponent: Component { FileDetailDialog {} }
    }

    Loader {
        id: previewDialogLoader
        active: false
        sourceComponent: Component { FilePreviewDialog {} }
    }

    Loader {
        id: shareDialogLoader
        active: false
        sourceComponent: Component { ShareDialog {} }
    }

    Loader {
        id: downloadHelperLoader
        active: false
        sourceComponent: Component { FileDownloadHelper { rootWindow: window } }
    }

    DangerConfirmDialog {
        id: dangerConfirmDialog
        onConfirmed: function (action, payload) {
            if (action === "trashSelected")
                FileController.moveSelectedToTrash();
            else if (action === "trashOne" && payload)
                FileController.moveToTrash(payload.fileId);
        }
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
                    root.confirmMoveToTrash(null, true);
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
                        onOpenFile: function (id, parentId, name) {
                            root.previewInfo({
                                fileId: id,
                                parentId: parentId,
                                fileName: name,
                                isFolder: false
                            });
                        }
                        onContextMenuRequested: function (index) {
                            var info = FileController.fileModel.getFileInfo(index);
                            contextMenu.targetIndex = index;
                            contextMenu.targetIsFolder = info.isFolder;
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
                        onOpenFile: function (id, parentId, name) {
                            root.previewInfo({
                                fileId: id,
                                parentId: parentId,
                                fileName: name,
                                isFolder: false
                            });
                        }
                        onContextMenuRequested: function (index) {
                            var info = FileController.fileModel.getFileInfo(index);
                            contextMenu.targetIndex = index;
                            contextMenu.targetIsFolder = info.isFolder;
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
