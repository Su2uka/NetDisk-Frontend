import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI
import App 1.0

Rectangle {
    id: root

    implicitHeight: 220
    Layout.minimumHeight: 140
    radius: 12
    color: "#ffffff"
    border.color: "#e8eaef"
    border.width: 0.5

    FluShadow {
        radius: 12
    }

    ColumnLayout {
        id: contentColumn
        anchors {
            fill: parent
            leftMargin: 20
            rightMargin: 20
            topMargin: 16
            bottomMargin: 16
        }
        spacing: 8

        FluText {
            text: "最近文件"
            font.pixelSize: 16
            font.bold: true
            color: "#1a1a2e"
            Layout.fillWidth: true
        }

        ListView {
            id: fileListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 56
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            model: RecentFilesManager.model
            visible: RecentFilesManager.model.count > 0

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: ItemDelegate {
                id: delegateItem
                width: fileListView.width
                height: 56
                leftPadding: 12
                rightPadding: 12
                topPadding: 4
                bottomPadding: 4

                background: Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    anchors.topMargin: 3
                    anchors.bottomMargin: 3
                    color: "#f5f5f5"
                    radius: 6
                    opacity: delegateItem.hovered || tapArea.containsMouse ? 1.0 : 0.0

                    Behavior on opacity {
                        NumberAnimation { duration: 120 }
                    }
                }

                contentItem: RowLayout {
                    spacing: 12

                    Item {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter

                        Rectangle {
                            id: recentThumbnailClip
                            anchors.fill: parent
                            radius: 5
                            clip: true
                            color: "transparent"
                            visible: recentThumbnail.status === Image.Ready

                            Image {
                                id: recentThumbnail
                                anchors.fill: parent
                                source: model.thumbnailUrl || ""
                                fillMode: Image.PreserveAspectCrop
                                sourceSize: Qt.size(56, 56)
                                asynchronous: true
                                cache: false
                            }
                        }

                        Image {
                            anchors.fill: parent
                            source: model.fileIcon
                            visible: !recentThumbnailClip.visible
                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(28, 28)
                        }
                    }

                    FluText {
                        text: model.fileName
                        font.pixelSize: 14
                        color: "#333333"
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    FluText {
                        text: model.fileTime
                        font.pixelSize: 13
                        color: "#999999"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                MouseArea {
                    id: tapArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("[RecentFiles] Clicked:", model.fileName)
                    onDoubleClicked: FileController.navigateToFileLocation(model.parentId || "")
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 4
                    height: 1
                    color: "#f0f0f0"
                    visible: index < RecentFilesManager.model.count - 1
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 72
            visible: RecentFilesManager.model.count === 0

            FluText {
                anchors.centerIn: parent
                text: "预览或下载文件后会显示在这里"
                font.pixelSize: 13
                color: "#8a8f9c"
            }
        }
    }
}
