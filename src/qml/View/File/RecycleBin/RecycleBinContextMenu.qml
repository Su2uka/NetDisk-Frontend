import QtQuick 2.15
import QtQuick.Controls 2.15
import FluentUI 1.0

FluMenu {
    id: root
    width: 140

    property int targetIndex: -1

    signal actionTriggered(string action, int index)

    FluMenuItem {
        text: "放回"
        onClicked: root.actionTriggered("restore", root.targetIndex)
    }
    FluMenuSeparator {}
    FluMenuItem {
        text: "删除"
        onClicked: root.actionTriggered("delete", root.targetIndex)
    }
}
