// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore
import MediaControls
import Config

Rectangle {
    id: root

    implicitWidth: 380
    color: Config.mainColor
    border.color: "lightgrey"
    radius: 10

    property int currentIndex: -1
    property bool isShuffled: false
    property alias mediaCount: files.count
    signal playlistUpdated()
    signal currentFileRemoved()
    signal playFileRequested(int fileIndex)

    function getSource() {
        if (isShuffled) {
            let randomIndex = Math.floor(Math.random() * mediaCount)
            while (randomIndex == currentIndex) {
                randomIndex = Math.floor(Math.random() * mediaCount)
            }
            currentIndex = randomIndex
        }
        return files.get(currentIndex).path
    }

    function getDisplayName() {
        if (currentIndex < 0 || currentIndex >= mediaCount)
            return ""
        var item = files.get(currentIndex)
        if (item.displayName.length > 0)
            return item.displayName
        var paths = item.path.split('/')
        return paths[paths.length - 1]
    }

    function addFiles(index, selectedFiles) {
        selectedFiles.forEach(function (file){
            const url = new URL(file)
            var urlStr = url.toString()
            files.insert(index,
                {
                    path: urlStr,
                    displayName: "",
                    isMovie: isMovie(urlStr)
                })
        })
        playlistUpdated()
    }

    function addFile(index, selectedFile, name) {
        if (index > mediaCount || index < 0) {
            index = 0
            currentIndex = 0
        }
        var fileStr = selectedFile.toString()
        files.insert(index,
            {
                path: fileStr,
                displayName: name || "",
                isMovie: isMovie(fileStr)
            })
    }

    function isMovie(path) {
        const paths = path.split('.')
        const extension = paths[paths.length - 1]
        const musicFormats = ["mp3", "wav", "aac"]
        for (const format of musicFormats) {
            if (format === extension) {
                return false
            }
        }
        return true
    }

    MouseArea {
        anchors.fill: root
        preventStealing: true
    }

    FileDialog {
        id: folderView
        title: qsTr("Add files to playlist")
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            root.addFiles(files.count, folderView.selectedFiles)
            close()
        }
    }

    ListModel {
        id: files
    }

    Item {
        id: playlist
        anchors.fill: root
        anchors.margins: 30

        RowLayout {
            id: header
            width: playlist.width

            Label {
                font.bold: true
                font.pixelSize: 20
                text: qsTr("Playlist")
                color: Config.secondaryColor

                Layout.fillWidth: true
            }

            CustomButton {
                icon.source: ControlImages.iconSource("Add_file")
                onClicked: folderView.open()
            }
        }

        ListView {
            id: listView
            model: files
            anchors.fill: playlist
            anchors.topMargin: header.height + 30
            spacing: 20

            delegate: Rectangle {
                id: row
                width: listView.width
                height: rowContent.implicitHeight + 8
                radius: 6
                color: rowMouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent"

                required property string path
                required property string displayName
                required property int index
                required property bool isMovie

                MouseArea {
                    id: rowMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentIndex = row.index
                        root.playFileRequested(row.index)
                    }
                }

                RowLayout {
                    id: rowContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 4
                    spacing: 15

                    Image {
                        id: mediaIcon

                        states: [
                            State {
                                name: "activeMovie"
                                when: root.currentIndex === row.index && row.isMovie
                                PropertyChanges {
                                    mediaIcon.source: Images.iconSource("Movie_Active", false)
                                }
                            },
                            State {
                                name: "inactiveMovie"
                                when: root.currentIndex !== row.index && row.isMovie
                                PropertyChanges {
                                    mediaIcon.source: Images.iconSource("Movie_Icon")
                                }
                            },
                            State {
                                name: "activeMusic"
                                when: root.currentIndex === row.index && !row.isMovie
                                PropertyChanges {
                                    mediaIcon.source: Images.iconSource("Music_Active", false)
                                }
                            },
                            State {
                                name: "inactiveMusic"
                                when: root.currentIndex !== row.index && !row.isMovie
                                PropertyChanges {
                                    mediaIcon.source: Images.iconSource("Music_Icon")
                                }
                            }
                        ]
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.bold: root.currentIndex === row.index
                        color: root.currentIndex === row.index ? "#2979FF" : Config.secondaryColor
                        font.pixelSize: 18
                        text: {
                            if (row.displayName.length > 0)
                                return row.displayName
                            const paths = row.path.split('/')
                            return paths[paths.length - 1]
                        }
                    }

                    CustomButton {
                        icon.source: ControlImages.iconSource("Trash_Icon")
                        onClicked: {
                            const removedIndex = row.index
                            files.remove(row.index)
                            if (root.currentIndex === removedIndex) {
                                root.currentFileRemoved()
                            } else if (root.currentIndex > removedIndex) {
                                --root.currentIndex
                            }
                        }
                    }
                }
            }

            remove: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 400
                }
            }

            add: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: 400
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.5
                    to: 1.0
                    duration: 400
                }
            }

            displaced: Transition {
                NumberAnimation {
                    properties: "y"
                    duration: 600
                    easing.type: Easing.OutBounce
                }
            }
        }
    }
}
