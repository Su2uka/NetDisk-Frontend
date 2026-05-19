import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import "../../Common"
import "../File/Components"
import App 1.0

/**
 * CategoryDetailPage — 智能目录详情页
 *
 * 由 HomePage 传入 categoryName 和 categoryKey，
 * 显示该分类下所有文件，支持子分类 PillTabBar 筛选。
 */
Item {
    id: root

    // ── 属性 ──
    property string categoryName: ""
    property string categoryKey: ""
    property int currentSubIndex: 0

    readonly property var subCategories: ({
            "image": ["全部", "JPG", "PNG", "GIF", "BMP", "SVG", "WEBP", "PSD", "其他"],
            "document": ["全部", "PDF", "DOC", "XLS", "PPT", "CAD", "TXT", "Pages", "Numbers", "Keynotes", "其他"],
            "video": ["全部", "MP4", "AVI", "MKV", "MOV", "WMV", "FLV", "其他"],
            "audio": ["全部", "MP3", "WAV", "FLAC", "AAC", "OGG", "WMA", "其他"]
        })

    readonly property var currentSubTabs: root.subCategories[root.categoryKey] || ["全部"]

    // ── 信号 ──
    signal goBack

    // ── 函数 ──
    function applyExtFilter() {
        if (root.currentSubIndex === 0) {
            FileController.filterCategoryByExt("");
        } else if (root.currentSubIndex === root.currentSubTabs.length - 1 && root.currentSubTabs[root.currentSubIndex] === "其他") {
            FileController.filterCategoryByExt("other");
        } else {
            FileController.filterCategoryByExt(root.currentSubTabs[root.currentSubIndex].toLowerCase());
        }
    }

    // ── 生命周期 ──
    onCurrentSubIndexChanged: root.applyExtFilter()
    Component.onCompleted: FileController.loadFilesByCategory(root.categoryKey)

    // ── 子对象 ──

    // 右键菜单
    FluMenu {
        id: contextMenu
        width: 180
        property int targetIndex: -1

        FluMenuItem {
            text: "前往文件位置"
            onClicked: {
                var info = FileController.fileModel.getFileInfo(contextMenu.targetIndex);
                FileController.navigateToFileLocation(info.parentId);
            }
        }
        FluMenuSeparator {}
        FluMenuItem {
            text: "下载"
            onClicked: {
                var info = FileController.fileModel.getFileInfo(contextMenu.targetIndex);
                downloadHelper.startDownload(info.fileId, info.fileName, info.fileSize);
            }
        }
        FluMenuItem {
            text: "分享"
            onClicked: console.log("分享:", contextMenu.targetIndex)
        }
        FluMenuItem {
            text: "查看详细信息"
            onClicked: {
                var info = FileController.fileModel.getFileInfo(contextMenu.targetIndex);
                fileDetailDialog.showDetail(info);
            }
        }
        FluMenuSeparator {}
        FluMenuItem {
            text: "放入回收站"
            textColor: "red"
            onClicked: {
                var info = FileController.fileModel.getFileInfo(contextMenu.targetIndex);
                FileController.moveToTrash(info.fileId);
            }
        }
    }

    FileDetailDialog {
        id: fileDetailDialog
    }

    FileDownloadHelper {
        id: downloadHelper
    }

    // 布局
    Column {
        anchors.fill: parent
        spacing: 0

        // 顶部栏
        Rectangle {
            width: parent.width
            height: 52
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 12

                FluIconButton {
                    iconSource: FluentIcons.Back
                    iconSize: 18
                    width: 32
                    height: 32
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: root.goBack()
                }

                FluText {
                    text: root.categoryName
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: "#1a1a2e"
                    Layout.alignment: Qt.AlignVCenter
                }

                PillTabBar {
                    tabLabels: root.currentSubTabs
                    currentIndex: root.currentSubIndex
                    onCurrentIndexChanged: root.currentSubIndex = currentIndex
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 4
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // 工具栏
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
                if (action === "删除")
                    FileController.moveSelectedToTrash();
                else
                    console.log("批量操作:", action);
            }
        }

        // 文件内容区域
        Item {
            width: parent.width
            height: parent.height - 96

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
                        onOpenFolder: function (id, name) { /* 分类视图不含文件夹 */ }
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
                        onOpenFolder: function (id, name) { /* 分类视图不含文件夹 */ }
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
        }
    }
}
