import QtQuick 2.15
import QtQuick.Dialogs
import FluentUI 1.0
import App 1.0

//  FileUploadHelper — 上传对话框 + 完成处理

Item {
    id: root

    // 暴露 root 窗口给 FluInfoBar（Overlay.overlay 需要）
    property var rootWindow: null

    // ── 对外暴露的接口 ──
    function openFileDialog() {
        fileDialog.open();
    }
    function openFolderDialog() {
        folderDialog.open();
    }

    // ── 上传完成后的信号（外部可选监听） ──
    signal uploadStarted(int fileCount)

    // ── 文件选择对话框（支持多选） ──
    FileDialog {
        id: fileDialog
        title: "选择要上传的文件"
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            var count = currentFiles.length;
            console.log("触发批量上传文件，共选中", count, "个文件");
            FileController.uploadFiles(currentFiles);
            root.uploadStarted(count);
        }
    }

    // ── 文件夹选择对话框 ──
    FolderDialog {
        id: folderDialog
        title: "选择要上传的文件夹"
        onAccepted: {
            console.log("触发上传文件夹，路径:", selectedFolder);
            FileController.uploadFolder(selectedFolder);
            root.uploadStarted(1);
        }
    }

    // ── 监听 UploadController 的完成/失败信号，做全局提示 ──
    Connections {
        target: UploadController

        function onTaskSuccess(taskId, fileName) {
            console.log("上传成功:", fileName);
            // 上传成功后自动刷新当前目录的文件列表
            FileController.loadFiles();
        }

        function onTaskFailed(taskId, fileName, errorMsg) {
            console.log("上传失败:", fileName, "原因:", errorMsg);
            if (infoBarLoader.item) {
                infoBarLoader.item.showError("上传失败：" + fileName, 4000, errorMsg);
            }
        }
    }

    // 懒加载 FluInfoBar（需要 rootWindow）
    Loader {
        id: infoBarLoader
        active: root.rootWindow !== null
        sourceComponent: FluInfoBar {
            property var root: rootWindow
            // 暴露 showError 给外部调用
        }
    }
}
