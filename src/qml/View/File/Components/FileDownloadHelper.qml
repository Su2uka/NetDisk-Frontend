import QtQuick 2.15
import QtQuick.Dialogs
import App 1.0

//  FileDownloadHelper — 下载路径选择 + 任务触发（支持单文件、文件夹、批量下载）

Item {
    id: root

    // ── 暂存待下载文件的信息 ──
    property string _fileId: ""
    property string _fileName: ""
    property real _fileSize: 0

    // ── 暂存待下载文件夹的信息 ──
    property string _folderId: ""
    property string _folderName: ""

    // ── 标记当前下载模式 ──
    // "file" = 单文件, "folder" = 文件夹, "batch" = 批量
    property string _downloadMode: "file"

    // ── 批量下载暂存列表 ──
    // 每项: { fileId, fileName, fileSize, isFolder }
    property var _batchItems: []

    // ── 对外接口：触发单文件下载流程 ──
    function startDownload(fileId, fileName, fileSize) {
        root._downloadMode = "file";
        root._fileId = fileId;
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
                startDownload(item.fileId, item.fileName, item.fileSize);
            }
            return;
        }

        root._downloadMode = "batch";
        root._batchItems = items;
        folderDialog.open();
    }

    // ── 目录选择对话框（所有下载模式共用）──
    FolderDialog {
        id: folderDialog
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
            // selectedFolder 是 QUrl，需要转为本地路径字符串
            var dirPath = selectedFolder.toString();

            // 去掉 file:/// 前缀（Windows 路径兼容）
            if (dirPath.startsWith("file:///")) {
                dirPath = dirPath.substring(8);
            }

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
                        FileController.requestDownload(bi.fileId, bi.fileName, bi.fileSize, dirPath);
                    }
                }
                root._batchItems = [];
                break;
            default:
                console.log("下载保存位置:", dirPath, "文件:", root._fileName);
                FileController.requestDownload(root._fileId, root._fileName, root._fileSize, dirPath);
                break;
            }
        }
    }

    // ── 监听 DownloadController 的完成/失败信号 ──
    Connections {
        target: DownloadController

        function onTaskSuccess(taskId, fileName) {
            console.log("下载成功:", fileName);
        }

        function onTaskFailed(taskId, fileName, errorMsg) {
            console.log("下载失败:", fileName, "原因:", errorMsg);
        }
    }
}
