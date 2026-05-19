import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts
import FluentUI 1.0
import QtQuick.Controls

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
                id: loginPanel
            }
            // Index 1: 注册
            RegisterPanel {
                id: registerPanel
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
                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }
                    TapHandler {
                        onTapped: {
                            if (swipeView.currentIndex === 0) {
                                registerPanel.clearForm();
                                swipeView.currentIndex = 1;
                            } else {
                                loginPanel.clearForm();
                                swipeView.currentIndex = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}
