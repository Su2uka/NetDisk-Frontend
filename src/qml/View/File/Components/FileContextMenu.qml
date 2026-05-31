import QtQuick 2.15
import FluentUI 1.0

FluMenu {
    id: root
    width: 180

    property int targetIndex: -1
    property bool targetIsFolder: false

    signal actionTriggered(string action, int index)

    FluMenuItem {
        text: "预览"
        iconSource: FluentIcons.Preview
        enabled: !root.targetIsFolder
        onClicked: root.actionTriggered("preview", root.targetIndex)
    }
    FluMenuItem {
        text: "分享"
        onClicked: root.actionTriggered("share", root.targetIndex)
    }
    FluMenuItem {
        text: "下载"
        onClicked: root.actionTriggered("download", root.targetIndex)
    }
    FluMenuItem {
        text: "收藏"
        onClicked: root.actionTriggered("favorite", root.targetIndex)
    }
    FluMenuItem {
        text: "重命名"
        onClicked: root.actionTriggered("rename", root.targetIndex)
    }
    FluMenuItem {
        text: "查看详细信息"
        onClicked: root.actionTriggered("detail", root.targetIndex)
    }

    FluMenuSeparator {}

    FluMenuItem {
        text: "放入回收站"
        textColor: "red"
        onClicked: root.actionTriggered("delete", root.targetIndex)
    }
}
