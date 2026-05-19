import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import App 1.0

FluWindow {
    id: splash
    width: 420
    height: 520
    fixSize: true
    showMaximize: false
    showMinimize: false
    showClose: false
    windowIcon: ""
    autoCenter: true
    backgroundColor: "#dedee1"

    // ── 业务逻辑 ──────────────────────────────

    property bool _autoLoginOk: false

    // 自动登录检查定时器
    Timer {
        id: timer_close
        interval: 1000
        onTriggered: LoginController.checkAutoLogin()
    }

    // 监听 LoginController 信号
    Connections {
        target: LoginController

        function onLoginSuccess(token) {
            console.log("自动登录成功, Token:", token);
            splash._autoLoginOk = true;
            anim_fade_out.start();
        }

        function onAutoLoginFailed() {
            console.log("自动登录失败, 跳转登录页");
            splash._autoLoginOk = false;
            anim_fade_out.start();
        }
    }

    // 淡出动画 (关闭时)
    NumberAnimation {
        id: anim_fade_out
        target: splash
        property: "opacity"
        from: 1
        to: 0
        duration: 400
        easing.type: Easing.InQuad
        onFinished: {
            if (splash._autoLoginOk) {
                FluRouter.navigate("/main");
            } else {
                FluRouter.navigate("/login");
            }
            splash.close();
        }
    }

    // 启动入场动画
    Component.onCompleted: splashLogo.start()

    //  UI 组件

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -20

            // Logo
            SplashLogo {
                id: splashLogo
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                onFinished: splashTitle.start()
            }

            Item {
                Layout.preferredHeight: 28
            }

            // 标题 + 副标题
            SplashTitle {
                id: splashTitle
                Layout.alignment: Qt.AlignHCenter
                onFinished: {
                    splashProgress.start();
                    timer_close.start();
                }
            }

            Item {
                Layout.preferredHeight: 50
            }

            // 进度条
            SplashProgressBar {
                id: splashProgress
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 180
            }
        }

        // 底部版本信息
        SplashVersionLabel {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
