import QtQuick
import QtQuick.Layouts
import FluentUI

ColumnLayout {
    id: root
    spacing: 0

    signal finished()

    function start() {
        anim_title.start()
    }

    // 主标题
    FluText {
        id: text_title
        Layout.alignment: Qt.AlignHCenter
        text: "AeroDrive"
        font.pixelSize: 32
        font.weight: Font.Bold
        font.letterSpacing: 2
        opacity: 0

        ParallelAnimation {
            id: anim_title
            NumberAnimation {
                target: text_title; property: "opacity"
                from: 0; to: 1; duration: 500
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: text_title; property: "y"
                from: text_title.y + 15; to: text_title.y
                duration: 500
                easing.type: Easing.OutCubic
            }
            onFinished: anim_subtitle.start()
        }
    }

    Item { Layout.preferredHeight: 8 }

    // 副标题
    FluText {
        id: text_subtitle
        Layout.alignment: Qt.AlignHCenter
        text: "轻云盘"
        font.pixelSize: 15
        font.letterSpacing: 6
        color: FluColors.Grey150
        opacity: 0

        ParallelAnimation {
            id: anim_subtitle
            NumberAnimation {
                target: text_subtitle; property: "opacity"
                from: 0; to: 1; duration: 400
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: text_subtitle; property: "y"
                from: text_subtitle.y + 10; to: text_subtitle.y
                duration: 400
                easing.type: Easing.OutCubic
            }
            onFinished: root.finished()
        }
    }
}
