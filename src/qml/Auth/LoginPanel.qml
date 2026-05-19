import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import QtQuick.Effects
import "../Common"
import "FormValidator.js" as FormValidator
import App 1.0

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.75
        spacing: 30

        // 1. Logo 区域
        Item {
            id: iconContainer
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 80
            Layout.preferredHeight: 80

            Image {
                id: sourceIcon
                visible: false
                anchors.fill: parent
                source: "qrc:/icon/res/icon/cloud-app-icon.svg"
                sourceSize: Qt.size(100, 100)
                fillMode: Image.PreserveAspectFit
            }

            //渲染带阴影的图像
            MultiEffect {
                anchors.fill: sourceIcon
                source: sourceIcon

                // 阴影配置
                shadowEnabled: true      // 开启阴影
                shadowColor: Qt.rgba(0, 0, 0, 0.4)  // 颜色：黑色，半透明 (alpha: 0.4)
                shadowBlur: 1.0  // 模糊半径：值越大阴影越柔和
                shadowHorizontalOffset: 0   // 水平偏移：0 表示居中
                shadowVerticalOffset: 5  // 垂直偏移：正数表示向下偏移，实现底部阴影效果
                shadowScale: 1.0  // 阴影缩放：1.0 为原大小，小于 1.0 会让阴影看起来稍微收缩一点
            }
        }

        // 2. 标题区
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5
            FluText {
                text: "欢迎回来"
                font: FluTextStyle.Title
                anchors.horizontalCenter: parent.horizontalCenter
            }
            FluText {
                text: "请登录您的账户以继续"
                color: FluColors.Grey100
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // 3. 输入框区
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20

            LeftIconTextBox {
                id: emailInput
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                iconSource: FluentIcons.Mail
                placeholderText: " 电子邮箱"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                LeftIconTextBox {
                    id: passwordInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    iconSource: FluentIcons.Lock
                    placeholderText: " 密码"
                    isPassword: true  // 开启密码模式
                }

                RowLayout {
                    FluRadioButton {
                        id: autoLoginCheck
                        text: "下次自动登录"
                        textColor: FluColors.Grey100
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    FluText {
                        text: "忘记密码?"
                        color: FluColors.Blue.normal
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignRight
                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }
        }

        // 4. 登录按钮
        FluFilledButton {
            text: "登录"
            font.pixelSize: 16

            Layout.fillWidth: true
            Layout.preferredHeight: 40

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            // 悬停上浮动画
            scale: hovered ? 1.01 : 1.0
            Behavior on scale {
                NumberAnimation {
                    duration: 100
                }
            }

            onClicked: {
                login();
            }
        }
    }

    function clearForm() {
        emailInput.text = "";
        passwordInput.text = "";
        autoLoginCheck.checked = false;
    }

    function login() {
        var email = emailInput.text.trim();
        var password = passwordInput.text;

        var emailResult = FormValidator.validateEmail(email);
        if (!emailResult.ok) {
            showWarning(emailResult.msg);
            return;
        }

        var pwdResult = FormValidator.validatePassword(password);
        if (!pwdResult.ok) {
            showWarning(pwdResult.msg);
            return;
        }

        console.log("正在登录: " + email);
        LoginController.login(email, password, autoLoginCheck.checked);
    }

    Connections {
        target: LoginController
        function onLoginSuccess(token) {
            // showSuccess("登录成功")
            console.log("Token: " + token);
            FluRouter.navigate("/main");
            window.close();
        }
        function onLoginFailed(errMsg) {
            showWarning(errMsg);
        }
    }
}
