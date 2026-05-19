import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

Rectangle {
    id: root
    width: parent.width
    height: 40
    color: "transparent"

    // ── 属性 ──
    required property var pathModel

    // ── 信号 ──
    signal navigateTo(int index)

    // ── 子对象 ──
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20

        Row {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter

            Repeater {
                model: root.pathModel

                Row {
                    spacing: 4

                    AbstractButton {
                        id: crumbBtn
                        width: crumbText.implicitWidth
                        height: crumbText.implicitHeight
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: index < root.pathModel.length - 1

                        onClicked: root.navigateTo(index)

                        // Qt5 AbstractButton 没有 cursorShape，用 MouseArea 代替
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: crumbBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onPressed: function (mouse) {
                                mouse.accepted = false;
                            }
                        }

                        contentItem: FluText {
                            id: crumbText
                            text: {
                                var node = root.pathModel[index];
                                return node.title !== undefined ? node.title : "未知";
                            }
                            font.pixelSize: 14
                            font.bold: index === root.pathModel.length - 1
                            color: crumbBtn.pressed ? "#868686" : crumbBtn.hovered ? "#5c5c5c" : "#1a1a1a"
                        }
                    }

                    FluText {
                        text: "/"
                        font.pixelSize: 14
                        visible: index < root.pathModel.length - 1
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
