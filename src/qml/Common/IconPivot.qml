import QtQuick 2.15
import QtQuick.Controls 2.15
import FluentUI 1.0

Item {
    id: root

    // 属性暴露
    property int currentIndex: 0
    property var tabModels: []

    implicitHeight: 53
    implicitWidth: navBar.contentWidth

    // 背景
    Rectangle {
        anchors.fill: parent
        anchors.bottomMargin: 3   // 给阴影留空间
        color: "#f3f4f8"
    }

    // 顶部 Pivot 导航栏
    Flickable {
        id: navBar
        width: parent.width
        height: 50
        contentWidth: navRow.width
        clip: true
        interactive: contentWidth > width

        Row {
            id: navRow
            height: parent.height
            spacing: 4
            leftPadding: 20
            rightPadding: 20

            Repeater {
                model: root.tabModels

                Button {
                    id: tabBtn
                    height: 36
                    anchors.verticalCenter: parent.verticalCenter
                    padding: 0
                    leftPadding: 14
                    rightPadding: 14

                    property bool isActive: root.currentIndex === index

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }

                    background: Rectangle {
                        // 选中态
                        color: {
                            if (tabBtn.isActive)
                                return "#e8e9ef";
                            if (tabBtn.pressed)
                                return "#0a000000";
                            if (tabBtn.hovered)
                                return "#05000000";
                            return "transparent";
                        }
                        radius: 8
                        border.width: tabBtn.isActive ? 0.5 : 0
                        border.color: "#d5d6dc"
                    }

                    contentItem: Row {
                        spacing: 7
                        anchors.verticalCenter: parent.verticalCenter

                        FluIcon {
                            iconSource: modelData.icon
                            iconSize: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: tabBtn.isActive ? "#333340" : "#8b8fa3"
                        }

                        FluText {
                            text: modelData.title
                            font.pixelSize: 13
                            font.weight: tabBtn.isActive ? Font.DemiBold : Font.Medium
                            anchors.verticalCenter: parent.verticalCenter
                            color: tabBtn.isActive ? "#1a1a2e" : "#5f6377"
                        }
                    }

                    onClicked: {
                        root.currentIndex = index;
                    }
                }
            }
        }
    }

    // 底部阴影渐变（替代硬分割线）
    Rectangle {
        width: parent.width
        height: 3
        anchors.top: navBar.bottom

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#0d000000"
            }
            GradientStop {
                position: 1.0
                color: "transparent"
            }
        }
    }
}
