import QtQuick
import FluentUI

Item {
    id: root
    width: 100
    height: 100

    signal finished()

    function start() {
        anim_logo.start()
    }

    Image {
        id: img_logo
        anchors.centerIn: parent
        width: 90
        height: 90
        source: "qrc:/icon/res/icon/cloud-app-icon.svg"
        sourceSize: Qt.size(90, 90)
        fillMode: Image.PreserveAspectFit
        antialiasing: true
        smooth: true
        opacity: 0
    }

    ParallelAnimation {
        id: anim_logo
        NumberAnimation {
            target: img_logo; property: "opacity"
            from: 0; to: 1; duration: 600
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: img_logo; property: "scale"
            from: 0.5; to: 1.0; duration: 700
            easing.type: Easing.OutBack
            easing.overshoot: 1.2
        }
        onFinished: root.finished()
    }
}
