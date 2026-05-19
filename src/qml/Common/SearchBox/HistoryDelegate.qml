import QtQuick 2.15
import QtQuick.Layouts 1.15
import FluentUI

Item {
    id: delegate
    width: ListView.view ? ListView.view.width : 0
    height: 44

    // ─── Model roles ───
    // 由 ListView 注入: model.text, model.index

    // ─── Signals ───
    signal searched(string keyword)
    signal deleted(int itemIndex)

    // ─── Hover 背景 ───
    Rectangle {
        anchors {
            fill: parent
            leftMargin: 6
            rightMargin: 6
            topMargin: 3
            bottomMargin: 3
        }
        radius: 4
        color: _mouseArea.containsMouse ? "#f5f5f5" : "transparent"
    }

    MouseArea {
        id: _mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: delegate.searched(model.text)
    }

    RowLayout {
        anchors {
            fill: parent
            leftMargin: 16
            rightMargin: 12
        }
        spacing: 12

        FluIcon {
            iconSource: FluentIcons.History
            iconSize: 16
            iconColor: "#888888"
            Layout.alignment: Qt.AlignVCenter
        }

        FluText {
            text: model.text
            font.pixelSize: 14
            color: "#333333"
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }

        FluIconButton {
            iconSource: FluentIcons.ChromeClose
            iconSize: 10
            iconColor: "#aaaaaa"
            width: 28
            height: 28
            Layout.alignment: Qt.AlignVCenter
            onClicked: delegate.deleted(model.index)
        }
    }
}
