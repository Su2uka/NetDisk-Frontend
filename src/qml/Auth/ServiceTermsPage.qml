import QtQuick 2.15
import QtQuick.Controls
import FluentUI

FluWindow {
    width: 1000
    height: 800

    title: "服务条款"
    launchMode: FluWindowType.SingleInstance

    // 滚动区域
    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.leftMargin: 30
        // anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.rightMargin: 10
        contentWidth: width
        contentHeight: contentText.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Text {
            id: contentText
            width: flickable.width
            leftPadding: 20
            rightPadding: 20
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
            color: FluTheme.fontPrimaryColor
            font.pixelSize: 14
            lineHeight: 1.6
            text: ""
        }

        ScrollBar.vertical: FluScrollBar {}
    }

    Component.onCompleted: {
        // 从 qrc 资源读取 HTML 文件内容
        var xhr = new XMLHttpRequest()
        xhr.open("GET", "qrc:/html/res/html/service.html", false)
        xhr.send()
        if (xhr.status === 200 || xhr.status === 0) {
            contentText.text = xhr.responseText
        }
    }
}
