import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI

// 这是一个通用的带图标输入框，支持普通模式和密码模式
Item {
    id: control

    // === 公开属性 ===
    property string text: ""
    property string placeholderText: ""
    property int iconSource: FluentIcons.Mail
    property int iconSize: 16
    property bool readOnly: false

    // === 核心功能开关 ===
    // 设置为 true 会自动启用密码隐藏和右侧的小眼睛按钮
    property bool isPassword: false

    property int echoMode: isPassword ? TextInput.Password : TextInput.Normal

    // === 样式属性 ===
    property color normalColor: FluTheme.dark ? Qt.rgba(45/255,45/255,45/255,1) : Qt.rgba(248/255,248/255,248/255,1)
    property color hoverColor: FluTheme.dark ? Qt.rgba(50/255,50/255,50/255,1) : Qt.rgba(243/255,243/255,243/255,1)
    property color focusColor: FluTheme.dark ? Qt.rgba(30/255,30/255,30/255,1) : Qt.rgba(255/255,255/255,255/255,1)
    property color borderColor: FluTheme.dark ? "#505050" : "#dfdfdf"
    property color focusBorderColor: FluTheme.primaryColor

    signal accepted()

    implicitWidth: 240
    implicitHeight: 34

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 4
        color: {
            if (input.activeFocus) return control.focusColor
            if (mouse_area.containsMouse) return control.hoverColor
            return control.normalColor
        }
        border.width: input.activeFocus ? 2 : 1
        border.color: input.activeFocus ? control.focusBorderColor : control.borderColor

        MouseArea {
            id: mouse_area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.IBeamCursor
            onClicked: input.forceActiveFocus()
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 1. 左侧图标
            Item {
                Layout.preferredWidth: 34
                Layout.fillHeight: true
                visible: control.iconSource !== 0 // 如果没设图标可隐藏

                FluIcon {
                    anchors.centerIn: parent
                    iconSource: control.iconSource
                    iconSize: control.iconSize
                    iconColor: input.activeFocus ? FluTheme.primaryColor : FluTheme.dark ? Qt.rgba(160/255,160/255,160/255,1) : Qt.rgba(96/255,96/255,96/255,1)
                }
            }

            // 2. 输入框本体
            TextField {
                id: input
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: control.text
                placeholderText: control.placeholderText
                readOnly: control.readOnly
                echoMode: control.echoMode // 自动根据属性切换

                background: Item {}
                padding: 0
                leftPadding: control.iconSource !== 0 ? 0 : 10 // 如果没图标，左边给点padding
                rightPadding: control.isPassword ? 0 : 10      // 如果有眼睛按钮，padding留给它
                verticalAlignment: TextInput.AlignVCenter

                font: FluTextStyle.Body
                color: FluTheme.fontPrimaryColor
                placeholderTextColor: FluTheme.fontSecondaryColor
                selectionColor: FluTheme.primaryColor
                selectedTextColor: "white"
                activeFocusOnTab: true

                onTextChanged: control.text = text
                onAccepted: control.accepted()
            }

            // 3. 右侧眼睛按钮 (仅在密码模式下显示)
            FluIconButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                Layout.alignment: Qt.AlignVCenter
                Layout.rightMargin: 2

                visible: control.isPassword && control.text !== ""

                // 自动切换图标：眼睛睁开/闭合
                iconSource: control.echoMode === TextInput.Password ? FluentIcons.RedEye : FluentIcons.Hide
                iconSize: 12

                onClicked: {
                    // 切换 echoMode
                    if (control.echoMode === TextInput.Password) {
                        control.echoMode = TextInput.Normal
                    } else {
                        control.echoMode = TextInput.Password
                    }
                }
            }
        }
    }
}
