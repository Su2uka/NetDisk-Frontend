import QtQuick 2.15
import QtQuick.Dialogs
import FluentUI 1.0
import App 1.0

//  FileDownloadHelper — 下载路径选择 + 任务触发（支持单文件、文件夹、批量下载）

Item {
    id: root

    // ── 对外暴露 root 窗口（给 FluInfoBar 的 Overlay.overlay 使用）──
    property var rootWindow: null

    // ── 暂存待下载文件的信息 ──
    property string _fileId: ""
    property string _parentId: ""
    property string _fileName: ""
    property real _fileSize: 0

    // ── 暂存待下载文件夹的信息 ──
    property string _folderId: ""
    property string _folderName: ""

    // ── 标记当前下载模式 ──
    // "file" = 单文件, "folder" = 文件夹, "batch" = 批量
    property string _downloadMode: "file"

    // ── 批量下载暂存列表 ──
    // 每项: { fileId, parentId, fileName, fileSize, isFolder }
    property var _batchItems: []

    // ── 对外接口：触发单文件下载流程 ──
    function startDownload(fileId, fileName, fileSize, parentId) {
        root._downloadMode = "file";
        root._fileId = fileId;
        root._parentId = parentId || "";
        root._fileName = fileName;
        root._fileSize = fileSize;
        folderDialog.open();
    }

    // ── 对外接口：触发文件夹下载流程 ──
    function startFolderDownload(folderId, folderName) {
        root._downloadMode = "folder";
        root._folderId = folderId;
        root._folderName = folderName;
        folderDialog.open();
    }

    // ── 对外接口：触发批量下载流程（只弹一次目录选择框）──
    // items: Array of { fileId, fileName, fileSize, isFolder }
    function startBatchDownload(items) {
        if (!items || items.length === 0)
            return;

        // 只有一个文件时直接走单文件/文件夹流程
        if (items.length === 1) {
            var item = items[0];
            if (item.isFolder) {
                startFolderDownload(item.fileId, item.fileName);
            } else {
                startDownload(item.fileId, item.fileName, item.fileSize, item.parentId || "");
            }
            return;
        }

        root._downloadMode = "batch";
        root._batchItems = items;
        folderDialog.open();
    }

    function localPathFromUrl(url) {
        if (url && url.toLocalFile) {
            var localPath = url.toLocalFile();
            if (localPath !== "")
                return localPath;
        }

        var text = url.toString();
        if (text.startsWith("file:///"))
            return decodeURIComponent(text.substring(8));
        if (text.startsWith("file://"))
            return decodeURIComponent(text.substring(7));
        return decodeURIComponent(text);
    }

    // ── 目录选择对话框（所有下载模式共用）──
    FolderDialog {
        id: folderDialog
        currentFolder: "file:///" + SettingsController.downloadDir
        title: {
            switch (root._downloadMode) {
            case "folder":
                return "选择文件夹下载保存位置";
            case "batch":
                return "选择批量下载保存位置";
            default:
                return "选择下载保存位置";
            }
        }
        onAccepted: {
            var dirPath = root.localPathFromUrl(selectedFolder);

            switch (root._downloadMode) {
            case "folder":
                console.log("文件夹下载保存位置:", dirPath, "文件夹:", root._folderName);
                FileController.downloadFolder(root._folderId, root._folderName, dirPath);
                break;
            case "batch":
                console.log("批量下载保存位置:", dirPath, "共", root._batchItems.length, "项");
                for (var i = 0; i < root._batchItems.length; i++) {
                    var bi = root._batchItems[i];
                    if (bi.isFolder) {
                        FileController.downloadFolder(bi.fileId, bi.fileName, dirPath);
                    } else {
                        FileController.requestDownload(bi.fileId, bi.parentId || "", bi.fileName, bi.fileSize, dirPath);
                    }
                }
                root._batchItems = [];
                break;
            default:
                console.log("下载保存位置:", dirPath, "文件:", root._fileName);
                FileController.requestDownload(root._fileId, root._parentId, root._fileName, root._fileSize, dirPath);
                break;
            }
        }
    }

    // ── 监听 DownloadController 的完成/失败信号 ──
    Connections {
        target: DownloadController

        function onTaskSuccess(taskId, fileName) {
            console.log("[DownloadHelper] 下载成功:", fileName);
        }

        function onTaskFailed(taskId, fileName, errorMsg) {
            console.log("[DownloadHelper] 下载失败:", fileName, "原因:", errorMsg);
            if (infoBarLoader.item && infoBarLoader.item.showError) {
                infoBarLoader.item.showError("下载失败：" + fileName, 4000, errorMsg);
            }
        }
    }

    // ── 懒加载 FluInfoBar（需要 rootWindow）──
    Loader {
        id: infoBarLoader
        active: root.rootWindow !== null
        sourceComponent: FluInfoBar {
            property var root: rootWindow
        }
    }
}
