import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import "../Common"
import "FormValidator.js" as FormValidator
import App 1.0

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.75
        spacing: 15

        //   1. 标题区
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5

            FluText {
                text: "创建新账号"
                font: FluTextStyle.Title
                anchors.horizontalCenter: parent.horizontalCenter
            }
            FluText {
                text: "免费注册，开启您的体验"
                color: FluColors.Grey100
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        //   2. 表单区
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // 电子邮箱
            LeftIconTextBox {
                id: emailInput
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                placeholderText: "电子邮箱"
                iconSource: FluentIcons.Mail
            }

            // 验证码区域
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // 验证码输入框
                LeftIconTextBox {
                    id: codeInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    placeholderText: "验证码"
                    iconSource: FluentIcons.Shield
                }

                // 获取验证码按钮 (带倒计时逻辑)
                FluButton {
                    id: btn_code
                    text: "获取验证码"
                    Layout.preferredHeight: 45
                    Layout.preferredWidth: 100
                    disabled: timer_code.running // 倒计时期间禁用

                    onClicked: {
                        if (getCode()) {
                            // 启动倒计时
                            timer_code.seconds = 60;
                            timer_code.start();
                        }
                    }

                    // 倒计时器
                    Timer {
                        id: timer_code
                        interval: 1000
                        repeat: true
                        property int seconds: 60
                        onTriggered: {
                            seconds--;
                            if (seconds <= 0) {
                                stop();
                                btn_code.text = "获取验证码";
                            } else {
                                btn_code.text = seconds + "s";
                            }
                        }
                    }
                }
            }

            // 密码框
            LeftIconTextBox {
                id: passwordInput
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                placeholderText: "设置密码"
                iconSource: FluentIcons.Lock
                isPassword: true // 开启密码模式
            }

            // 确认密码框
            LeftIconTextBox {
                id: confirmPasswordInput
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                placeholderText: "确认密码"
                iconSource: FluentIcons.Lock
                isPassword: true
            }
        }

        //   3. 服务条款
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            FluCheckBox {
                id: check_agreement
                text: ""
            }

            FluText {
                text: "我已阅读并同意"
                color: FluColors.Grey100
                font.pixelSize: 12
            }

            FluText {
                text: "《服务条款》"
                color: FluTheme.primaryColor
                font.pixelSize: 12
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: FluRouter.navigate("/term")
                }
            }
        }

        //   4. 注册按钮
        FluFilledButton {
            id: registerButton

            text: "立即注册"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.topMargin: 10

            normalColor: FluColors.Grey160
            hoverColor: FluColors.Grey150
            textColor: "white"

            disabled: !check_agreement.checked   // 只有勾选了协议才启用

            onClicked: {
                register();
            }

            FluTooltip {
                visible: registerButton.hovered && !check_agreement.checked
                text: "请先同意服务条款"
                delay: 300
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            // 点击缩放反馈
            scale: registerButton.pressed ? 0.98 : (registerButton.hovered ? 1.02 : 1.0)
            Behavior on scale {
                NumberAnimation {
                    duration: 100
                }
            }
        }
    }

    // ========== 业务逻辑 ==========

    function clearForm() {
        emailInput.text = "";
        codeInput.text = "";
        passwordInput.text = "";
        confirmPasswordInput.text = "";
        check_agreement.checked = false;
    }

    function getCode() {
        var email = emailInput.text.trim();

        var result = FormValidator.validateEmail(email);
        if (!result.ok) {
            showWarning(result.msg);
            return false;
        }

        LoginController.getVerificationCode(email);
        return true;
    }

    function register() {
        var email = emailInput.text.trim();
        var code = codeInput.text.trim();
        var password = passwordInput.text;
        var confirmPassword = confirmPasswordInput.text;

        // 1. 验证邮箱
        var emailResult = FormValidator.validateEmail(email);
        if (!emailResult.ok) {
            showWarning(emailResult.msg);
            return;
        }

        // 2. 验证验证码非空
        if (code === "") {
            showWarning("请输入验证码");
            return;
        }

        // 3. 验证密码
        var pwdResult = FormValidator.validatePassword(password);
        if (!pwdResult.ok) {
            showWarning(pwdResult.msg);
            return;
        }

        // 4. 验证确认密码
        var confirmResult = FormValidator.validateConfirmPassword(password, confirmPassword);
        if (!confirmResult.ok) {
            showWarning(confirmResult.msg);
            return;
        }

        // 5. 发送注册请求
        LoginController.registerUser(email, code, password);
    }

    // ========== 信号监听 ==========

    Connections {
        target: LoginController
        function onVerificationCodeSent() {
            showSuccess("验证码已发送，请查收邮箱");
        }
        function onVerificationCodeFailed(errMsg) {
            showWarning(errMsg);
        }
        function onRegisterSuccess() {
            showSuccess("注册成功");
            var email = emailInput.text.trim();
            var password = passwordInput.text;
            LoginController.login(email, password, false);
        }
        function onRegisterFailed(errMsg) {
            showWarning(errMsg);
        }
    }
}
