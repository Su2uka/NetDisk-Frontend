import QtQuick
import FluentUI

FluText {
    id: root
    text: "Version 1.0.0"
    font.pixelSize: 11
    color: FluColors.Grey100
    opacity: 0

    NumberAnimation on opacity {
        from: 0; to: 1; duration: 800
        easing.type: Easing.OutCubic
        running: true
    }
}
