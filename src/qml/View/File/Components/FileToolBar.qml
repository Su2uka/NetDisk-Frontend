import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

Rectangle {
    id: root
    width: parent.width
    height: 44
    color: "transparent"

    // ── 外部状态 ──
    property int selectedCount: 0
    property int totalCount: 0
    property bool hasSelection: false
    property int sortField: 1
    property bool sortAsc: false
    property int viewMode: 1

    // ── 信号 ──
    signal selectAll
    signal clearSelection
    signal requestSortField(int field)
    signal requestSortAsc(bool asc)
    signal viewModeToggled
    signal batchAction(string action)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20

        // 全选
        FluCheckBox {
            id: selectAllCheck
            text: "全选"
            textColor: FluColors.Grey100
            checked: root.selectedCount > 0
            indeterminate: root.selectedCount > 0 && root.selectedCount < root.totalCount
            clickListener: function () {
                if (root.selectedCount === root.totalCount && root.totalCount > 0) {
                    root.clearSelection();
                } else {
                    root.selectAll();
                }
            }
            Layout.alignment: Qt.AlignVCenter
        }

        // ── 批量操作按钮 ──
        Row {
            visible: root.hasSelection
            spacing: 4
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 12

            Repeater {
                model: [
                    {
                        label: "下载",
                        icon: FluentIcons.Download
                    },
                    {
                        label: "分享",
                        icon: FluentIcons.Share
                    },
                    {
                        label: "收藏",
                        icon: FluentIcons.FavoriteStar
                    },
                    {
                        label: "删除",
                        icon: FluentIcons.Delete
                    }
                ]

                FluIconButton {
                    display: Button.TextBesideIcon
                    text: modelData.label
                    iconSource: modelData.icon
                    iconSize: 14
                    font.pixelSize: 12
                    implicitWidth: 72
                    onClicked: root.batchAction(modelData.label)
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        // 排序 + 视图切换
        Row {
            Layout.alignment: Qt.AlignVCenter
            spacing: 8

            FluIconButton {
                id: sortBtn
                display: Button.TextBesideIcon
                iconSource: FluentIcons.Sort
                iconColor: FluColors.Grey100
                iconSize: 14
                font.pixelSize: 13
                implicitWidth: 140
                text: {
                    var fields = ["名称", "创建时间", "修改时间", "文件大小"];
                    return "按" + fields[root.sortField] + "排序";
                }
                textColor: FluColors.Grey100

                onClicked: sortMenu.open()

                FluMenu {
                    id: sortMenu
                    width: 160

                    FluMenuItem {
                        text: "名称"
                        iconSource: root.sortField === 0 ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortField(0)
                    }
                    FluMenuItem {
                        text: "创建时间"
                        iconSource: root.sortField === 1 ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortField(1)
                    }
                    FluMenuItem {
                        text: "修改时间"
                        iconSource: root.sortField === 2 ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortField(2)
                    }
                    FluMenuItem {
                        text: "文件大小"
                        iconSource: root.sortField === 3 ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortField(3)
                    }

                    FluMenuSeparator {}

                    FluMenuItem {
                        text: "升序"
                        iconSource: root.sortAsc ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortAsc(true)
                    }
                    FluMenuItem {
                        text: "降序"
                        iconSource: !root.sortAsc ? FluentIcons.CheckMark : 0
                        onClicked: root.requestSortAsc(false)
                    }
                }
            }

            // 切换视图按钮
            FluIconButton {
                id: viewToggleBtn
                iconSource: root.viewMode === 0 ? FluentIcons.List : FluentIcons.ViewAll
                iconColor: FluColors.Grey100
                iconSize: 15
                onClicked: root.viewModeToggled()
            }
        }
    }
}
