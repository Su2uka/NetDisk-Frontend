pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import FluentUI

Rectangle {
    id: root

    // ── 信号 ──
    signal categoryClicked(string categoryName, string categoryKey)

    // ── 属性 ──
    height: 120
    radius: 12
    color: "#ffffff"
    border.color: "#e8eaef"
    border.width: 0.5

    // ── 子对象 ──
    FluShadow {
        radius: 12
    }

    Column {
        anchors {
            fill: parent
            leftMargin: 20
            rightMargin: 20
            topMargin: 16
            bottomMargin: 16
        }
        spacing: 14

        FluText {
            text: "智能目录"
            font.pixelSize: 16
            font.bold: true
            color: "#1a1a2e"
        }

        Row {
            id: categoryRow

            width: parent.width
            spacing: 12

            Repeater {
                model: ListModel {
                    ListElement {
                        label: "图片"
                        categoryKey: "image"
                        iconCode: 0xEB9F
                        bgColor: "#FFF3E0"
                        iconColor: "#FB8C00"
                    }
                    ListElement {
                        label: "文档"
                        categoryKey: "document"
                        iconCode: 0xE8A5
                        bgColor: "#E8F5E9"
                        iconColor: "#43A047"
                    }
                    ListElement {
                        label: "视频"
                        categoryKey: "video"
                        iconCode: 0xE8B2
                        bgColor: "#F3E5F5"
                        iconColor: "#8E24AA"
                    }
                    ListElement {
                        label: "音频"
                        categoryKey: "audio"
                        iconCode: 0xE8D6
                        bgColor: "#FCE4EC"
                        iconColor: "#E91E63"
                    }
                }

                delegate: AbstractButton {
                    id: categoryBtn

                    required property string label
                    required property string categoryKey
                    required property int iconCode
                    required property color bgColor
                    required property color iconColor

                    width: (categoryRow.width - 3 * categoryRow.spacing) / 4
                    height: 45

                    onClicked: root.categoryClicked(categoryBtn.label, categoryBtn.categoryKey)

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: function (mouse) {
                            mouse.accepted = false;
                        }
                    }

                    background: Rectangle {
                        radius: 8
                        color: categoryBtn.hovered ? Qt.darker(categoryBtn.bgColor, 1.05) : "#f5f5f5"
                        border.color: "#e8e8e8"
                        border.width: 1

                        Behavior on color {
                            ColorAnimation {
                                duration: 150
                            }
                        }
                    }

                    contentItem: Item {
                        anchors.fill: parent

                        Row {
                            anchors.centerIn: parent
                            spacing: 15

                            Rectangle {
                                width: 30
                                height: 30
                                radius: 6
                                color: categoryBtn.bgColor
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    anchors.centerIn: parent
                                    font.family: "Segoe Fluent Icons"
                                    font.pixelSize: 14
                                    text: String.fromCharCode(categoryBtn.iconCode)
                                    color: categoryBtn.iconColor
                                }
                            }

                            FluText {
                                text: categoryBtn.label
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: "#333333"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
