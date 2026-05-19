import QtQuick 2.15
import QtQuick.Controls 2.15
import FluentUI 1.0

/**
 * PillTabBar — 可复用的药丸形 Tab 栏
 *
 * 使用方式:
 *   PillTabBar {
 *       tabLabels: ["上传", "下载", "已完成"]
 *       currentIndex: 0
 *       onCurrentIndexChanged: { ... }
 *   }
 */
Row {
    id: root

    property var tabLabels: []          // Tab 名称列表，如 ["上传", "下载", "已完成"]
    property int currentIndex: 0        // 当前选中 Tab 索引

    spacing: 6

    Repeater {
        model: root.tabLabels

        Rectangle {
            id: tabRect

            property int tabIndex: index
            property bool isSelected: root.currentIndex === tabIndex

            width: tabLabel.implicitWidth + 24
            height: 30
            radius: 15
            color: isSelected ? "#25262b" : (tabHover.hovered ? "#0a000000" : "transparent")
            border.width: isSelected ? 0 : 1
            border.color: isSelected ? "transparent" : "#e0e0e0"

            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }

            HoverHandler {
                id: tabHover
                cursorShape: Qt.PointingHandCursor
            }

            FluText {
                id: tabLabel
                anchors.centerIn: parent
                text: modelData
                font.pixelSize: 13
                font.weight: tabRect.isSelected ? Font.DemiBold : Font.Normal
                color: tabRect.isSelected ? "#ffffff" : "#555555"
            }

            TapHandler {
                onTapped: root.currentIndex = tabRect.tabIndex
            }
        }
    }
}
