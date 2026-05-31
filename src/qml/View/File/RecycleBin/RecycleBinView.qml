import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../Components"
import App 1.0

Item {
    id: root

    function confirmPermanentDelete(info, isBatch) {
        if (isBatch) {
            dangerConfirmDialog.openDialog(
                        "永久删除",
                        "确定要永久删除选中的 " + RecycleBin.selectedCount + " 个项目吗？此操作无法撤销。",
                        "永久删除",
                        "deleteSelected",
                        null);
            return;
        }

        dangerConfirmDialog.openDialog(
                    "永久删除",
                    "确定要永久删除「" + info.fileName + "」吗？此操作无法撤销。",
                    "永久删除",
                    "deleteOne",
                    info);
    }

    function confirmEmptyTrash() {
        if (RecycleBin.trashModel.count <= 0)
            return;

        dangerConfirmDialog.openDialog(
                    "清空回收站",
                    "确定要清空回收站中的 " + RecycleBin.trashModel.count + " 个项目吗？此操作无法撤销。",
                    "清空回收站",
                    "emptyTrash",
                    null);
    }

    // ── 生命周期 ──
    Component.onCompleted: Qt.callLater(RecycleBin.loadTrash)

    // ── 子对象 ──

    Connections {
        target: RecycleBin
        function onTrashOpSuccess(message) {
            showSuccess(message);
        }
    }

    RecycleBinContextMenu {
        id: contextMenu
        onActionTriggered: function (action, index) {
            var info = RecycleBin.trashModel.getFileInfo(index);
            if (action === "restore") {
                if (RecycleBin.selectedCount > 1)
                    RecycleBin.restoreSelected();
                else
                    RecycleBin.restoreFile(info.fileId);
            } else if (action === "delete") {
                if (RecycleBin.selectedCount > 1)
                    root.confirmPermanentDelete(info, true);
                else
                    root.confirmPermanentDelete(info, false);
            }
        }
    }

    DangerConfirmDialog {
        id: dangerConfirmDialog
        onConfirmed: function (action, payload) {
            if (action === "deleteSelected")
                RecycleBin.deleteSelected();
            else if (action === "deleteOne" && payload)
                RecycleBin.permanentDelete(payload.fileId);
            else if (action === "emptyTrash")
                RecycleBin.emptyTrash();
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // 工具栏
        RecycleBinToolBar {
            selectedCount: RecycleBin.selectedCount
            totalCount: RecycleBin.trashModel.count
            hasSelection: RecycleBin.hasSelection
            viewMode: RecycleBin.viewMode

            onSelectAll: RecycleBin.selectAll()
            onClearSelection: RecycleBin.clearSelection()
            onViewModeToggled: RecycleBin.viewMode = RecycleBin.viewMode === 0 ? 1 : 0
            onBatchAction: function (action) {
                if (action === "放回")
                    RecycleBin.restoreSelected();
                else if (action === "删除")
                    root.confirmPermanentDelete(null, true);
                else if (action === "清空")
                    root.confirmEmptyTrash();
                else if (action === "恢复全部")
                    RecycleBin.restoreAll();
            }
        }

        // 文件区域
        Item {
            width: parent.width
            height: parent.height - 96

            Loader {
                anchors.fill: parent
                active: RecycleBin.viewMode === 0
                sourceComponent: Component {
                    FileGridView {
                        anchors.fill: parent
                        model: RecycleBin.trashModel
                        hasSelection: RecycleBin.hasSelection
                        onToggleSelection: function (index) {
                            RecycleBin.toggleSelection(index);
                        }
                        onOpenFolder: function (id, name) { /* 回收站不支持打开文件夹 */ }
                        onContextMenuRequested: function (index) {
                            contextMenu.targetIndex = index;
                            contextMenu.popup();
                        }
                    }
                }
            }

            Loader {
                anchors.fill: parent
                active: RecycleBin.viewMode === 1
                sourceComponent: Component {
                    RecycleBinListView {
                        anchors.fill: parent
                        fileModel: RecycleBin.trashModel
                        hasSelection: RecycleBin.hasSelection
                        onToggleSelection: function (index) {
                            RecycleBin.toggleSelection(index);
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
                text: "暂无回收站文件"
                font.pixelSize: 14
                color: "#bbbbbb"
                visible: FavoritesController.fileModel.count === 0
            }

            FluMenu {
                id: blankAreaMenu
                width: 160
                FluMenuItem {
                    text: "清空回收站"
                    onClicked: root.confirmEmptyTrash()
                }
                FluMenuItem {
                    text: "刷新页面"
                    onClicked: RecycleBin.loadTrash()
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
