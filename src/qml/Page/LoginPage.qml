import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts
import FluentUI 1.0
import QtQuick.Controls

import "../Component/Login"

FluWindow {
    id: window
    width: 450
    height: 600

    fixSize: true
    showMaximize: false
    windowIcon: ""

    backgroundColor: "#dedee1"

    ColumnLayout {
           anchors.fill: parent
           spacing: 0
           SwipeView {
               id: swipeView
               Layout.fillWidth: true
               Layout.fillHeight: true
               interactive: false // 禁止手势滑动
               clip: true
               // Index 0: 登录
               LoginPanel {
                   onRequestRegister: swipeView.currentIndex = 1
               }
               // Index 1: 注册
               RegisterPanel {
                   onRequestLogin: swipeView.currentIndex = 0
               }
           }
           // 底部引导区
           Rectangle {
               Layout.fillWidth: true
               Layout.preferredHeight: 60
               color: "transparent"

               Row {
                   anchors.centerIn: parent
                   spacing: 5

                   FluText {
                       text: swipeView.currentIndex === 0 ? "还没有账号？" : "已有账号？"
                       color: FluColors.Grey100
                   }

                   FluText {
                       text: swipeView.currentIndex === 0 ? "立即注册" : "直接登录"
                       color: FluColors.Blue.normal
                       font.bold: true
                       HoverHandler { cursorShape: Qt.PointingHandCursor }
                       TapHandler {
                           onTapped: {
                            swipeView.currentIndex = (swipeView.currentIndex === 0 ? 1 : 0)
                           }
                       }
                   }
               }
           }
       }
   }
