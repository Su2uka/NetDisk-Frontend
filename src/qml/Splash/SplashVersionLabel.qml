import QtQuick
import FluentUI

FluText {
    id: root

    property real fadeOpacity: 0

    text: "Version 1.0.0"
    font.pixelSize: 11
    color: FluColors.Grey100
    opacity: fadeOpacity

    Component.onCompleted: fadeIn.start()

    NumberAnimation {
        id: fadeIn
        target: root
        property: "fadeOpacity"
        from: 0; to: 1; duration: 800
        easing.type: Easing.OutCubic
    }
}
