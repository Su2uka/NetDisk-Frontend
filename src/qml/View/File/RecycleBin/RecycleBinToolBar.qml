import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

Rectangle {
    id: root
    width: parent.width
    height: 48
    color: "transparent"

    // ── 外部状态 ──
    property int selectedCount: 0
    property int totalCount: 0
    property bool hasSelection: false
    property int viewMode: 0

    // ── 信号 ──
    signal selectAll
    signal clearSelection
    signal viewModeToggled
    signal batchAction(string action)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 12

        // ── 左侧：全选 + 计数 + 批量操作 ──
        FluCheckBox {
            size: 16
            checked: root.selectedCount > 0 && root.selectedCount === root.totalCount
            clickListener: function () {
                if (root.hasSelection) {
                    root.clearSelection();
                } else {
                    root.selectAll();
                }
            }
        }

        FluText {
            text: root.hasSelection ? "已选 " + root.selectedCount + " 项" : "共 " + root.totalCount + " 项"
            font.pixelSize: 13
            color: "#888888"
        }

        // 批量放回
        FluTextButton {
            text: "放回"
            visible: root.hasSelection
            font.pixelSize: 13
            onClicked: root.batchAction("放回")
        }

        // 批量删除
        FluTextButton {
            text: "删除"
            visible: root.hasSelection
            font.pixelSize: 13
            onClicked: root.batchAction("删除")
        }

        Item {
            Layout.fillWidth: true
        }

        // ── 右侧：清空 + 恢复全部 + 视图切换 ──
        FluTextButton {
            text: "清空回收站"
            font.pixelSize: 13
            onClicked: root.batchAction("清空")
        }

        FluTextButton {
            text: "恢复全部"
            font.pixelSize: 13
            onClicked: root.batchAction("恢复全部")
        }

        FluIconButton {
            iconSource: root.viewMode === 0 ? FluentIcons.GridView : FluentIcons.List
            iconSize: 16
            onClicked: root.viewModeToggled()
        }
    }

    // 底部分割线
    Rectangle {
        width: parent.width - 40
        height: 1
        color: "#f0f0f0"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
