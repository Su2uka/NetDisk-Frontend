import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import FluentUI
import App 1.0

FluWindow {
    id: window
    width: 1050
    height: 700
    minimumWidth: 1050
    minimumHeight: 700
    windowIcon: ""
    backgroundColor: "#fafafa"
    Component.onCompleted: {
        appBar.color = "#e8e8e9";
        // 恢复未完成的传输任务
        UploadController.restorePendingUploads();
        DownloadController.restorePendingDownloads();
        // 拉取用户信息（存储概览等）
        UserController.fetchUserInfo();
        // 登录成功进入主页后，才启用剪贴板口令嗅探
        ClipboardController.startMonitoring();
    }

    // ── 剪贴板分享口令嗅探弹窗 ──
    ShareSniffDialog {
        id: shareSniffDialog
    }

    // ── 分享保存弹窗 ──
    ShareSaveDialog {
        id: shareSaveDialog
    }

    // ── 全局消息提示条 ──
    FluInfoBar {
        id: infoBar
        root: window
    }

    Connections {
        target: ClipboardController
        function onShareCodeDetected(shareKey, extractCode) {
            shareSniffDialog.openDialog(shareKey, extractCode);
        }
        function onShareVerified(shareToken, sharerName, sharerAvatar, fileName, fileId, isFolder) {
            shareSniffDialog.close();
            shareSaveDialog.openDialog(shareToken, sharerName, sharerAvatar, fileName, fileId, isFolder);
        }
        function onShareVerifyFailed(errorMsg) {
            shareSniffDialog.close();
            infoBar.showWarning("口令验证失败：" + errorMsg, 3000);
        }
        function onOwnShareDetected() {
            shareSniffDialog.close();
            infoBar.showInfo("这是你自己的分享，无需转存哦~", 3000);
        }
    }

    Connections {
        target: LoginController
        function onAutoLoginFailed() {
            infoBar.showWarning("登录已过期，请重新登录", 2000);
            FluRouter.navigate("/login");
            window.close();
        }
        function onLogoutSuccess() {
            FluRouter.navigate("/login");
            window.close();
        }
    }

    // 侧边栏
    Sidebar {
        id: sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        onPageSelected: function (url) {
            pageLoader.source = url;
        }
    }

    // 工具栏
    TopBar {
        id: searchBar
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        onSearchClicked: function (keyword) {
            console.log("搜索关键词:", keyword);
        }
    }

    // 页面内容区域：侧边栏右侧、工具栏下方
    Rectangle {
        id: contentArea
        color: "#f3f4f8"
        anchors {
            left: sidebar.right
            right: parent.right
            top: searchBar.bottom
            bottom: parent.bottom
        }

        // 监听 FileController 的"前往文件位置"信号
        Connections {
            target: FileController
            function onGoToFileLocationRequested(folderId) {
                // folderId 已在 C++ 端 navigateToFileLocation() 中写入 m_pendingBreadcrumbs，
                // AllFilesView.onCompleted 会自动调用 loadFiles()，通过 currentFolderId() 读取。
                pageLoader.source = "qrc:/qml/View/File/FilePage.qml";
                sidebar.navigationView.setCurrentIndex(1); // 切换侧边栏到"文件"
            }
        }

        // 当前默认加载 Home 页面
        Loader {
            id: pageLoader
            anchors.fill: parent
            source: "qrc:/qml/View/Home/HomePage.qml"
        }
    }
}
