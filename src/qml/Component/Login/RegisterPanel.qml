import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import "../Common"

Item {
   // 信号：请求切换回登录页面
    signal requestLogin()

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
                        // 启动倒计时
                        timer_code.seconds = 60
                        timer_code.start()
                        // TODO: 这里触发实际的发送验证码逻辑
                        showSuccess("验证码已发送")
                    }

                    // 倒计时器
                    Timer {
                        id: timer_code
                        interval: 1000
                        repeat: true
                        property int seconds: 60
                        onTriggered: {
                            seconds--
                            if(seconds <= 0) {
                                stop()
                                btn_code.text = "获取验证码"
                            } else {
                                btn_code.text = seconds + "s"
                            }
                        }
                    }
                }
            }

            // 密码框
            LeftIconTextBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                placeholderText: "设置密码"
                iconSource: FluentIcons.Lock
                isPassword: true // 开启密码模式
            }

            // 确认密码框
            LeftIconTextBox {
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
                    onClicked: showInfo("打开服务条款...")
                }
            }
        }

        //   4. 注册按钮
        FluFilledButton {
            text: "立即注册"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.topMargin: 10

            // 注册按钮使用深灰色，与登录区分
            normalColor: FluColors.Grey160
            hoverColor: FluColors.Grey150
            textColor: "white"

            // 只有勾选了协议才启用
            disabled: !check_agreement.checked

            onClicked: {
                showSuccess("注册请求已提交")
            }

            // 简单的点击缩放反馈
            scale: pressed ? 0.98 : (hovered ? 1.02 : 1.0)
            Behavior on scale { NumberAnimation { duration: 100 } }
        }
    }
}
