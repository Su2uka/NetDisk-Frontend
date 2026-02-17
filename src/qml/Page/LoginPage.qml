import QtQuick 2.15
import QtQuick.Window 2.15
import FluentUI 1.0

FluWindow {
    id: window
    width: 450
    height: 600

    fixSize: true
    showMaximize: false
    windowIcon: ""

    // 你的窗口内容
    Column {
        anchors.centerIn: parent
        spacing: 20

        FluText {
            text: "这是一个固定大小的无边框窗口"
            font: FluTextStyle.Title
        }

        FluText {
            text: "你无法调整此窗口的大小，且标题栏没有最大化按钮。"
            font: FluTextStyle.Body
        }
    }
}
