import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import FluentUI
import App 1.0

FluWindow {
    id: window
    width: 1050
    height: 650
    minimumWidth: 1050
    minimumHeight: 650
    windowIcon: ""
    backgroundColor: "#fafafa"
    property bool transfersRestored: false

    Component.onCompleted: {
        appBar.color = "#e8e8e9";
        // 先拉取用户信息，拿到 userId 后再恢复用户隔离的数据。
        UserController.fetchUserInfo();
        // 登录成功进入主页后，才启用剪贴板口令嗅探。
        ClipboardController.startMonitoring();
    }

    Connections {
        target: UserController
        function onUserInfoChanged() {
            if (window.transfersRestored || UserController.userId <= 0)
                return;
            window.transfersRestored = true;
            RecentFilesManager.reloadForCurrentUser();
            TransferHistory.reloadForCurrentUser();
            UploadController.restorePendingUploads();
            DownloadController.restorePendingDownloads();
        }
    }

    // 剪贴板分享口令嗅探弹窗
    ShareSniffDialog {
        id: shareSniffDialog
    }

    // 分享保存弹窗
    ShareSaveDialog {
        id: shareSaveDialog
    }

    // 全局消息提示条
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
            infoBar.showInfo("这是你自己的分享，无需转存", 3000);
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
            if (url === "qrc:/qml/View/File/FilePage.qml") {
                FileController.clearSearch();
                Qt.callLater(FileController.loadFiles);
            }
            pageLoader.source = url;
        }
    }

    // 工具栏
    TopBar {
        id: searchBar
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        onFileSelected: function (fileInfo) {
            if (!fileInfo)
                return;
            var targetFolderId = fileInfo.isFolder ? fileInfo.fileId : fileInfo.parentId;
            FileController.navigateToFileLocation(targetFolderId || "");
        }
        onSettingsRequested: {
            pageLoader.source = "qrc:/qml/View/Settings/SettingsPage.qml";
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
                pageLoader.source = "qrc:/qml/View/File/FilePage.qml";
                sidebar.navigationView.setCurrentIndex(1);
            }
        }

        Loader {
            id: pageLoader
            anchors.fill: parent
            source: "qrc:/qml/View/Home/HomePage.qml"
            onLoaded: {
                if (pageLoader.prevItem && pageLoader.prevItem.settingsBackRequested) {
                    pageLoader.prevItem.settingsBackRequested.disconnect(pageLoader._onSettingsBack);
                }
                if (item && item.settingsBackRequested) {
                    item.settingsBackRequested.connect(pageLoader._onSettingsBack);
                }
                pageLoader.prevItem = item;
            }
            property var prevItem: null
            property var _onSettingsBack: function() {
                pageLoader.source = "qrc:/qml/View/Home/HomePage.qml";
            }
        }
    }
}
