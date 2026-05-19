import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import FluentUI 1.0
import App 1.0

/**
 * ShareSaveDialog — 分享提取 & 保存到网盘弹窗
 *
 * 验证成功后弹出，显示分享信息 + 文件列表 + 树形文件夹选择器，
 * 用户选择目标文件夹后一键保存到自己的网盘。
 */
FluContentDialog {
    id: root

    title: ""
    implicitWidth: 520
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "取消"
    positiveText: "保存到网盘"

    // ── 外部传入的分享数据 ──
    property string _shareToken: ""
    property string _sharerName: ""
    property string _sharerAvatar: ""
    property string _fileName: ""
    property int    _fileId: 0
    property bool   _isFolder: false

    // ── 选中的目标文件夹 ──
    property int    _targetFolderId: 0
    property string _targetFolderName: "全部文件"

    function openDialog(shareToken, sharerName, sharerAvatar, fileName, fileId, isFolder) {
        root._shareToken = shareToken;
        root._sharerName = sharerName;
        root._sharerAvatar = sharerAvatar || "";
        root._fileName = fileName;
        root._fileId = fileId;
        root._isFolder = isFolder;
        root._targetFolderId = 0;
        root._targetFolderName = "全部文件";

        folderTreeModel.clear();
        folderTreeModel.append({
            "folderId":   0,
            "folderName": "全部文件",
            "level":      0,
            "expanded":   true,
            "loaded":     false
        });

        // 初始化分享文件树
        shareTreeModel.clear();
        shareTreeModel.append({
            "fileId":   fileId,
            "name":     fileName,
            "isFolder": isFolder,
            "size":     0,
            "level":    0,
            "expanded": isFolder,
            "loaded":   !isFolder
        });

        if (isFolder) {
            ClipboardController.loadShareFiles(fileId);
        }
        ClipboardController.loadMyFolders(0);
        root.open();
    }

    onPositiveClicked: {
        ClipboardController.saveToMyDisk([root._fileId], root._targetFolderId);
    }

    // ══════════════════════════════════════
    //  树形模型
    // ══════════════════════════════════════
    ListModel { id: folderTreeModel }
    ListModel { id: shareTreeModel }

    Connections {
        target: ClipboardController

        function onFolderChildrenLoaded(parentId, children) {
            var parentIndex = -1;
            for (var i = 0; i < folderTreeModel.count; i++) {
                if (folderTreeModel.get(i).folderId === parentId) {
                    parentIndex = i;
                    break;
                }
            }
            if (parentIndex < 0) return;

            var parentLevel = folderTreeModel.get(parentIndex).level;
            folderTreeModel.setProperty(parentIndex, "loaded", true);

            var insertPos = parentIndex + 1;
            for (var j = 0; j < children.length; j++) {
                folderTreeModel.insert(insertPos + j, {
                    "folderId":   children[j].fileId,
                    "folderName": children[j].name,
                    "level":      parentLevel + 1,
                    "expanded":   false,
                    "loaded":     false
                });
            }
        }

        function onShareChildrenLoaded(parentId, children) {
            var parentIndex = -1;
            for (var i = 0; i < shareTreeModel.count; i++) {
                if (parentId === -1) {
                    if (shareTreeModel.get(i).level === 0) {
                        parentIndex = i;
                        break;
                    }
                } else {
                    if (shareTreeModel.get(i).fileId === parentId) {
                        parentIndex = i;
                        break;
                    }
                }
            }
            if (parentIndex < 0) return;

            var parentLevel = shareTreeModel.get(parentIndex).level;
            shareTreeModel.setProperty(parentIndex, "loaded", true);

            var insertPos = parentIndex + 1;
            for (var j = 0; j < children.length; j++) {
                shareTreeModel.insert(insertPos + j, {
                    "fileId":   children[j].fileId,
                    "name":     children[j].name,
                    "isFolder": children[j].isFolder,
                    "size":     children[j].size,
                    "level":    parentLevel + 1,
                    "expanded": false,
                    "loaded":   !children[j].isFolder
                });
            }
        }

        function onSaveSuccess(message) {
            root.close();
        }

        function onSaveFailed(errorMsg) {
            console.log("[分享保存] 失败:", errorMsg);
        }
    }

    function expandFolder(idx) {
        var item = folderTreeModel.get(idx);
        folderTreeModel.setProperty(idx, "expanded", true);
        if (!item.loaded) {
            ClipboardController.loadMyFolders(item.folderId);
        }
    }

    function collapseFolder(idx) {
        folderTreeModel.setProperty(idx, "expanded", false);
        var parentLevel = folderTreeModel.get(idx).level;
        var removeStart = idx + 1;
        var removeCount = 0;
        while (removeStart + removeCount < folderTreeModel.count) {
            if (folderTreeModel.get(removeStart + removeCount).level > parentLevel) {
                removeCount++;
            } else {
                break;
            }
        }
        if (removeCount > 0) {
            folderTreeModel.remove(removeStart, removeCount);
        }
        folderTreeModel.setProperty(idx, "loaded", false);
    }

    function expandShareNode(idx) {
        var item = shareTreeModel.get(idx);
        shareTreeModel.setProperty(idx, "expanded", true);
        if (!item.loaded) {
            ClipboardController.loadShareFiles(item.fileId);
        }
    }

    function collapseShareNode(idx) {
        shareTreeModel.setProperty(idx, "expanded", false);
        var parentLevel = shareTreeModel.get(idx).level;
        var removeStart = idx + 1;
        var removeCount = 0;
        while (removeStart + removeCount < shareTreeModel.count) {
            if (shareTreeModel.get(removeStart + removeCount).level > parentLevel) {
                removeCount++;
            } else {
                break;
            }
        }
        if (removeCount > 0) {
            shareTreeModel.remove(removeStart, removeCount);
        }
        shareTreeModel.setProperty(idx, "loaded", false);
    }

    // ══════════════════════════════════════
    //  内容区
    // ══════════════════════════════════════
    contentDelegate: Component {
        Item {
            implicitHeight: mainCol.implicitHeight + 20

            ColumnLayout {
                id: mainCol
                width: parent.width
                spacing: 0

                // ═══════════════════
                //  头部：分享者信息
                // ═══════════════════
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24; Layout.rightMargin: 24
                    Layout.topMargin: 4;  Layout.bottomMargin: 16
                    spacing: 14

                    // 头像
                    FluClip {
                        radius: [23, 23, 23, 23]
                        width: 46; height: 46
                        Layout.alignment: Qt.AlignVCenter

                        // 默认头像
                        Image {
                            anchors.fill: parent
                            source: "qrc:/img/res/image/User.svg"
                            sourceSize: Qt.size(92, 92)
                            fillMode: Image.PreserveAspectCrop
                            visible: sharerAvatarImg.status !== Image.Ready
                        }

                        // 网络头像（异步加载，就绪后覆盖默认）
                        Image {
                            id: sharerAvatarImg
                            asynchronous: true
                            anchors.fill: parent
                            source: root._sharerAvatar
                            sourceSize: Qt.size(92, 92)
                            fillMode: Image.PreserveAspectCrop
                            visible: status === Image.Ready
                        }
                    }

                    Column {
                        spacing: 2; Layout.alignment: Qt.AlignVCenter; Layout.fillWidth: true
                        FluText {
                            text: root._sharerName + " 的分享"
                            font.pixelSize: 15; font.weight: Font.DemiBold
                        }
                        FluText {
                            text: "选择保存位置，一键转存到你的网盘"
                            font.pixelSize: 12; color: "#888"
                        }
                    }
                }

                // 分隔线
                Rectangle { Layout.fillWidth: true; height: 1; color: FluTheme.dividerColor }

                // ═══════════════════
                //  分享文件预览
                // ═══════════════════
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24; Layout.rightMargin: 24
                    Layout.topMargin: 14; Layout.bottomMargin: 10
                    spacing: 8

                    FluText {
                        text: "分享内容"
                        font.pixelSize: 13; font.weight: Font.Medium; color: "#555"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        radius: 6
                        color: Qt.rgba(0,0,0,0.02)
                        border.width: 1
                        border.color: FluTheme.dividerColor
                        clip: true

                        ListView {
                            id: shareTreeView
                            anchors.fill: parent
                            anchors.margins: 4
                            model: shareTreeModel
                            spacing: 0
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: FluScrollBar {}

                            FluText {
                                anchors.centerIn: parent
                                text: ClipboardController.loading ? "加载中..." : "暂无文件"
                                font.pixelSize: 12; color: "#bbb"
                                visible: shareTreeView.count === 0
                            }

                            delegate: Item {
                                id: shareDelegate
                                width: shareTreeView.width
                                height: 34

                                readonly property int    itemFileId:   (model.fileId   !== undefined && model.fileId   !== null) ? model.fileId   : -1
                                readonly property string itemName:     (model.name     !== undefined && model.name     !== null) ? model.name     : ""
                                readonly property bool   itemIsFolder: (model.isFolder !== undefined && model.isFolder !== null) ? model.isFolder : false
                                readonly property var    itemSize:     (model.size     !== undefined && model.size     !== null) ? model.size     : 0
                                readonly property int    itemLevel:    (model.level    !== undefined && model.level    !== null) ? model.level    : 0
                                readonly property bool   itemExpanded: (model.expanded !== undefined && model.expanded !== null) ? model.expanded : false
                                readonly property bool   isHovered:    shareItemMa.containsMouse

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.leftMargin: 2; anchors.rightMargin: 2
                                    anchors.topMargin: 1; anchors.bottomMargin: 1
                                    radius: 4
                                    color: shareDelegate.isHovered ? Qt.rgba(0,0,0,0.03) : "transparent"
                                    Behavior on color { ColorAnimation { duration: 80 } }
                                }

                                MouseArea {
                                    id: shareItemMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8 + shareDelegate.itemLevel * 20
                                    anchors.rightMargin: 8
                                    spacing: 0

                                    // 展开/折叠箭头（仅文件夹）
                                    Item {
                                        Layout.preferredWidth: 24
                                        Layout.preferredHeight: 24
                                        Layout.alignment: Qt.AlignVCenter
                                        visible: shareDelegate.itemIsFolder

                                        FluIcon {
                                            anchors.centerIn: parent
                                            iconSource: FluentIcons.ChevronRight
                                            iconSize: 10
                                            iconColor: "#999"
                                            rotation: shareDelegate.itemExpanded ? 90 : 0
                                            Behavior on rotation {
                                                NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (shareDelegate.itemExpanded) {
                                                    root.collapseShareNode(index);
                                                } else {
                                                    root.expandShareNode(index);
                                                }
                                            }
                                        }
                                    }

                                    // 非文件夹占位
                                    Item {
                                        Layout.preferredWidth: 24
                                        visible: !shareDelegate.itemIsFolder
                                    }

                                    // 图标
                                    FluIcon {
                                        Layout.leftMargin: 2
                                        iconSource: shareDelegate.itemIsFolder
                                                    ? (shareDelegate.itemExpanded ? FluentIcons.OpenFolderHorizontal : FluentIcons.Folder)
                                                    : FluentIcons.Document
                                        iconSize: 16
                                        iconColor: shareDelegate.itemIsFolder ? "#F5A623" : "#4A90D9"
                                    }

                                    // 名称
                                    FluText {
                                        Layout.leftMargin: 8
                                        Layout.fillWidth: true
                                        text: shareDelegate.itemName
                                        font.pixelSize: 13
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }

                // 分隔线
                Rectangle { Layout.fillWidth: true; height: 1; color: FluTheme.dividerColor }

                // ═══════════════════
                //  树形文件夹选择器
                // ═══════════════════
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 24; Layout.rightMargin: 24
                    Layout.topMargin: 14; Layout.bottomMargin: 8
                    spacing: 8

                    FluText {
                        text: "保存到"
                        font.pixelSize: 13; font.weight: Font.Medium; color: "#555"
                    }

                    // 树形容器 —— 固定高度，内部滚动
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: 6
                        color: Qt.rgba(0,0,0,0.02)
                        border.width: 1
                        border.color: FluTheme.dividerColor
                        clip: true

                        ListView {
                            id: treeView
                            anchors.fill: parent
                            anchors.margins: 4
                            model: folderTreeModel
                            spacing: 0
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: FluScrollBar {}

                            // 空状态
                            FluText {
                                anchors.centerIn: parent
                                text: ClipboardController.loading ? "加载中..." : "当前目录没有子文件夹"
                                font.pixelSize: 12; color: "#bbb"
                                visible: treeView.count === 0
                            }

                            delegate: Item {
                                id: treeDelegate
                                width: treeView.width
                                height: 34

                                // 安全取值
                                readonly property int    itemFolderId: (model.folderId   !== undefined && model.folderId   !== null) ? model.folderId   : -1
                                readonly property string itemName:     (model.folderName !== undefined && model.folderName !== null) ? model.folderName : ""
                                readonly property int    itemLevel:    (model.level      !== undefined && model.level      !== null) ? model.level      : 0
                                readonly property bool   itemExpanded: (model.expanded   !== undefined && model.expanded   !== null) ? model.expanded   : false
                                readonly property bool   isSelected:   itemFolderId === root._targetFolderId
                                readonly property bool   isHovered:    delegateMa.containsMouse

                                // 选中指示条（左侧竖条，WinUI NavigationView 风格）
                                Rectangle {
                                    width: 3; height: 16; radius: 1.5
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: FluTheme.primaryColor
                                    visible: treeDelegate.isSelected
                                }

                                // 行背景
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.leftMargin: 2; anchors.rightMargin: 2
                                    anchors.topMargin: 1; anchors.bottomMargin: 1
                                    radius: 4
                                    color: {
                                        if (treeDelegate.isSelected)
                                            return Qt.rgba(0,0,0,0.06);
                                        if (treeDelegate.isHovered)
                                            return Qt.rgba(0,0,0,0.03);
                                        return "transparent";
                                    }
                                    Behavior on color { ColorAnimation { duration: 80 } }
                                }

                                MouseArea {
                                    id: delegateMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root._targetFolderId = treeDelegate.itemFolderId;
                                        root._targetFolderName = treeDelegate.itemName;
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8 + treeDelegate.itemLevel * 20
                                    anchors.rightMargin: 8
                                    spacing: 0

                                    // 展开/折叠箭头
                                    Item {
                                        Layout.preferredWidth: 24
                                        Layout.preferredHeight: 24
                                        Layout.alignment: Qt.AlignVCenter

                                        FluIcon {
                                            anchors.centerIn: parent
                                            iconSource: FluentIcons.ChevronRight
                                            iconSize: 10
                                            iconColor: "#999"
                                            rotation: treeDelegate.itemExpanded ? 90 : 0

                                            Behavior on rotation {
                                                NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (treeDelegate.itemExpanded) {
                                                    root.collapseFolder(index);
                                                } else {
                                                    root.expandFolder(index);
                                                }
                                            }
                                        }
                                    }

                                    // 文件夹图标
                                    FluIcon {
                                        Layout.leftMargin: 2
                                        iconSource: treeDelegate.itemExpanded
                                                    ? FluentIcons.OpenFolderHorizontal
                                                    : FluentIcons.Folder
                                        iconSize: 16
                                        iconColor: treeDelegate.isSelected
                                                   ? FluTheme.primaryColor
                                                   : "#F5A623"
                                    }

                                    // 文件夹名
                                    FluText {
                                        Layout.leftMargin: 8
                                        Layout.fillWidth: true
                                        text: treeDelegate.itemName
                                        font.pixelSize: 13
                                        font.weight: treeDelegate.isSelected ? Font.DemiBold : Font.Normal
                                        color: treeDelegate.isSelected ? FluTheme.primaryColor : "#333"
                                        elide: Text.ElideRight
                                    }

                                    // 选中对勾
                                    FluIcon {
                                        iconSource: FluentIcons.CheckMark
                                        iconSize: 12
                                        iconColor: FluTheme.primaryColor
                                        visible: treeDelegate.isSelected
                                        Layout.rightMargin: 4
                                    }
                                }
                            }
                        }
                    }

                    // 当前选中提示
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6

                        FluIcon {
                            iconSource: FluentIcons.Folder
                            iconSize: 12
                            iconColor: FluTheme.primaryColor
                        }

                        FluText {
                            text: "将保存到：" + root._targetFolderName
                            font.pixelSize: 11; color: "#888"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
