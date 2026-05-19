import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0
import App 1.0

Item {
    id: root

    Component.onCompleted: Qt.callLater(MyShareController.loadShares)

    Connections {
        target: MyShareController
        function onShareOpSuccess(message) {
            showSuccess(message);
        }
        function onShareOpFailed(message) {
            showError(message);
        }
    }

    // ── 取消分享确认弹窗 ──
    FluContentDialog {
        id: cancelConfirmDialog
        title: "取消分享"
        message: "取消分享后，该链接会从列表里删除，且无法继续查看，是否继续？"
        buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
        negativeText: "再想想"
        positiveText: "确认取消"

        property int pendingShareId: -1
        property bool isBatch: false

        onPositiveClicked: {
            if (isBatch) {
                MyShareController.cancelSelected();
            } else if (pendingShareId >= 0) {
                MyShareController.cancelShare(pendingShareId);
            }
        }
    }

    // ── 右键菜单 ──
    FluMenu {
        id: contextMenu
        width: 160
        property int targetIndex: -1

        FluMenuItem {
            text: "复制分享码"
            iconSource: FluentIcons.Copy
            onClicked: {
                MyShareController.getShareCode(contextMenu.targetIndex);
            }
        }
        FluMenuSeparator {}
        FluMenuItem {
            text: "取消分享"
            iconSource: FluentIcons.Cancel
            textColor: "#e74c3c"
            onClicked: {
                var info = MyShareController.shareModel.getShareInfo(contextMenu.targetIndex);
                cancelConfirmDialog.pendingShareId = info.shareId;
                cancelConfirmDialog.isBatch = false;
                cancelConfirmDialog.open();
            }
        }
    }

    // ── 主布局 ──
    Column {
        anchors.fill: parent
        spacing: 0

        // ═══════════════════
        //  工具栏
        // ═══════════════════
        Rectangle {
            id: toolBar
            width: parent.width
            height: 44
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                // 全选
                FluCheckBox {
                    text: "全选"
                    textColor: FluColors.Grey100
                    checked: MyShareController.selectedCount > 0
                    indeterminate: MyShareController.selectedCount > 0
                                   && MyShareController.selectedCount < MyShareController.shareModel.count
                    clickListener: function () {
                        if (MyShareController.selectedCount === MyShareController.shareModel.count
                                && MyShareController.shareModel.count > 0) {
                            MyShareController.clearSelection();
                        } else {
                            MyShareController.selectAll();
                        }
                    }
                    Layout.alignment: Qt.AlignVCenter
                }

                // ── 批量操作按钮（选中时显示） ──
                Row {
                    visible: MyShareController.hasSelection
                    spacing: 4
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 12

                    FluIconButton {
                        display: Button.TextBesideIcon
                        text: "取消分享"
                        iconSource: FluentIcons.Cancel
                        iconSize: 14
                        font.pixelSize: 12
                        onClicked: {
                            cancelConfirmDialog.isBatch = true;
                            cancelConfirmDialog.pendingShareId = -1;
                            cancelConfirmDialog.open();
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // ── 排序按钮 ──
                Row {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8

                    FluIconButton {
                        display: Button.TextBesideIcon
                        iconSource: FluentIcons.Sort
                        iconColor: FluColors.Grey100
                        iconSize: 14
                        font.pixelSize: 13
                        text: {
                            var fields = ["创建时间", "名称", "浏览量", "保存量"];
                            return "按" + fields[MyShareController.sortField] + "排序";
                        }
                        textColor: FluColors.Grey100
                        onClicked: sortMenu.open()

                        FluMenu {
                            id: sortMenu
                            width: 160

                            FluMenuItem {
                                text: "创建时间"
                                iconSource: MyShareController.sortField === 0 ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortField = 0
                            }
                            FluMenuItem {
                                text: "名称"
                                iconSource: MyShareController.sortField === 1 ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortField = 1
                            }
                            FluMenuItem {
                                text: "浏览量"
                                iconSource: MyShareController.sortField === 2 ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortField = 2
                            }
                            FluMenuItem {
                                text: "保存量"
                                iconSource: MyShareController.sortField === 3 ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortField = 3
                            }

                            FluMenuSeparator {}

                            FluMenuItem {
                                text: "升序"
                                iconSource: MyShareController.sortAsc ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortAsc = true
                            }
                            FluMenuItem {
                                text: "降序"
                                iconSource: !MyShareController.sortAsc ? FluentIcons.CheckMark : 0
                                onClicked: MyShareController.sortAsc = false
                            }
                        }
                    }
                }
            }
        }

        // ═══════════════════
        //  列表区域
        // ═══════════════════
        Item {
            width: parent.width
            height: parent.height - toolBar.height

            // ── 表头 ──
            Rectangle {
                id: headerRow
                width: parent.width
                height: 40
                color: "transparent"
                z: 1

                Rectangle {
                    width: parent.width - 40
                    height: 1
                    color: "#eeeeee"
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Rectangle {
                    width: parent.width - 40
                    height: 1
                    color: "#eeeeee"
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 16

                    Item { width: 44; height: 1 }

                    FluText {
                        text: "名称"
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }

                    FluText {
                        text: "创建时间"
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                    }

                    FluText {
                        text: "浏览量"
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 70
                    }

                    FluText {
                        text: "保存量"
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 70
                    }

                    FluText {
                        text: "状态"
                        color: "#888888"
                        font.pixelSize: 12
                        Layout.preferredWidth: 110
                    }
                }
            }

            // ── 数据列表 ──
            ListView {
                id: shareList
                anchors.top: headerRow.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                clip: true
                model: MyShareController.shareModel
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOff
                }

                // 空状态
                FluText {
                    anchors.centerIn: parent
                    text: "暂无分享记录"
                    font.pixelSize: 14
                    color: "#bbbbbb"
                    visible: shareList.count === 0
                }

                delegate: Item {
                    width: shareList.width
                    height: 50

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        color: {
                            if (model.selected) return Qt.rgba(0, 0, 0, 0.04);
                            if (rowHover.hovered) return Qt.rgba(0, 0, 0, 0.02);
                            return "transparent";
                        }
                        radius: 6

                        HoverHandler {
                            id: rowHover
                        }

                        // 分割线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#e8e8e8"
                            anchors.bottom: parent.bottom
                            visible: index < MyShareController.shareModel.count - 1
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 16

                            // 复选框
                            Item {
                                width: 32
                                height: parent.height

                                FluCheckBox {
                                    anchors.centerIn: parent
                                    visible: MyShareController.hasSelection || rowHover.hovered
                                    checked: model.selected
                                    size: 16
                                    clickListener: function () {
                                        MyShareController.toggleSelection(index);
                                    }
                                }
                            }

                            // 文件信息 (图标+名称)
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Image {
                                    source: model.fileIcon
                                    width: 24
                                    height: 24
                                    sourceSize: Qt.size(24, 24)
                                }

                                FluText {
                                    text: model.fileName
                                    font.pixelSize: 13
                                    color: "#222222"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            // 创建时间
                            FluText {
                                text: {
                                    var raw = model.createdAt;
                                    if (!raw) return "-";
                                    var d = new Date(raw);
                                    return Qt.formatDateTime(d, "yyyy-MM-dd hh:mm");
                                }
                                color: "#888888"
                                font.pixelSize: 12
                                Layout.preferredWidth: 150
                            }

                            // 浏览量
                            FluText {
                                text: model.viewCount
                                color: "#888888"
                                font.pixelSize: 12
                                Layout.preferredWidth: 70
                            }

                            // 保存量
                            FluText {
                                text: model.saveCount
                                color: "#888888"
                                font.pixelSize: 12
                                Layout.preferredWidth: 70
                            }

                            // 状态
                            Rectangle {
                                Layout.preferredWidth: 110
                                Layout.preferredHeight: 24
                                color: {
                                    if (model.status === "cancelled") return "#FFEBEE";
                                    if (model.status === "expired")   return "#FFF3E0";
                                    return "#E8F5E9";
                                }
                                radius: 4
                                Layout.alignment: Qt.AlignVCenter

                                FluText {
                                    anchors.centerIn: parent
                                    text: {
                                        if (model.status === "cancelled") return "已取消";
                                        if (model.status === "expired")   return "已过期";
                                        // active
                                        if (!model.expireAt || model.expireAt === "") return "永久有效";
                                        var now  = new Date();
                                        var exp  = new Date(model.expireAt);
                                        var diff = Math.ceil((exp.getTime() - now.getTime()) / 86400000);
                                        if (diff <= 0) return "已过期";
                                        return "还有" + diff + "天过期";
                                    }
                                    font.pixelSize: 11
                                    color: {
                                        if (model.status === "cancelled") return "#C62828";
                                        if (model.status === "expired")   return "#E65100";
                                        if (!model.expireAt || model.expireAt === "") return "#2E7D32";
                                        var now2  = new Date();
                                        var exp2  = new Date(model.expireAt);
                                        var diff2 = Math.ceil((exp2.getTime() - now2.getTime()) / 86400000);
                                        if (diff2 <= 0) return "#E65100";
                                        if (diff2 <= 3) return "#E65100";
                                        return "#2E7D32";
                                    }
                                }
                            }
                        }

                        // 右键菜单
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: function (mouse) {
                                if (mouse.button === Qt.RightButton) {
                                    contextMenu.targetIndex = index;
                                    contextMenu.popup();
                                }
                            }
                        }
                    }
                }
            }

            // 空白区域右键菜单
            FluMenu {
                id: blankAreaMenu
                width: 160
                FluMenuItem {
                    text: "刷新页面"
                    onClicked: MyShareController.loadShares()
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
