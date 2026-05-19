import QtQuick
import FluentUI

FluProgressBar {
    id: root
    indeterminate: true
    opacity: 0

    function start() {
        anim_progress.start()
    }

    NumberAnimation {
        id: anim_progress
        target: root; property: "opacity"
        from: 0; to: 1; duration: 300
    }
}
