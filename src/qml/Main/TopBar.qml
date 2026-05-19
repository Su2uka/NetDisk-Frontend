import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import FluentUI
import App 1.0
import "../Common/SearchBox"

Rectangle {
    id: root
    height: 70
    color: "#ffffff"

    // 信号转发
    signal searchClicked(string keyword)
    signal fileSelected(int fileId)

    // 暴露 searchBox 以便外部调用 loadCloudResults()
    property alias searchBox: searchBox

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10

        SearchBox {
            id: searchBox
            Layout.preferredWidth: 360
            Layout.preferredHeight: 36
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 30

            onSearchTriggered: function (keyword) {
                root.searchClicked(keyword);
            }
            onFileSelected: function (fileId) {
                root.fileSelected(fileId);
            }
            onCloudSearchRequested: function (keyword) {
                console.log("[SearchBox] 云端搜索请求:", keyword);
            // TODO: 接入后端 API，回调 searchBox.loadCloudResults(jsonArray)
            }
        }

        Item {
            Layout.fillWidth: true
        }

        // 用户头像区域（阴影 + 点击菜单）
        Item {
            id: avatarWrapper
            width: 44
            height: 44
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: 30

            // 圆形阴影
            Repeater {
                model: 4
                Rectangle {
                    anchors.centerIn: parent
                    width: avatarClip.width + (index + 1) * 2
                    height: avatarClip.height + (index + 1) * 2
                    radius: width / 2
                    color: "transparent"
                    border.width: 1
                    border.color: "#999999"
                    opacity: 0.08 * (4 - index)
                }
            }

            // 头像
            FluClip {
                id: avatarClip
                radius: [20, 20, 20, 20]
                width: 40
                height: 40
                anchors.centerIn: parent

                // 默认占位头像
                Image {
                    id: placeholderAvatar
                    anchors.fill: parent
                    source: "qrc:/img/res/image/User.svg"
                    sourceSize: Qt.size(80, 80)
                    fillMode: Image.PreserveAspectCrop
                    visible: networkAvatar.status !== Image.Ready
                }

                // 网络头像（异步加载，就绪后覆盖占位图）
                Image {
                    id: networkAvatar
                    asynchronous: true
                    cache: false          // 禁用磁盘缓存，避免缓存损坏导致加载失败
                    anchors.fill: parent
                    source: UserController.avatar !== "" ? UserController.avatar : ""
                    sourceSize: Qt.size(80, 80)
                    fillMode: Image.PreserveAspectCrop
                    visible: status === Image.Ready

                    property int retryCount: 0
                    readonly property int maxRetries: 3

                    // 加载失败时自动重试
                    onStatusChanged: {
                        if (status === Image.Error && retryCount < maxRetries) {
                            retryTimer.start();
                        }
                    }

                    Timer {
                        id: retryTimer
                        interval: 1000 * (networkAvatar.retryCount + 1)  // 递增延迟: 1s, 2s, 3s
                        repeat: false
                        onTriggered: {
                            networkAvatar.retryCount++;
                            var baseUrl = UserController.avatar;
                            if (baseUrl !== "") {
                                // 添加时间戳参数绕过缓存
                                var separator = baseUrl.indexOf("?") === -1 ? "?" : "&";
                                networkAvatar.source = baseUrl + separator + "_t=" + Date.now();
                                console.log("[TopBar] 头像加载重试 (" + networkAvatar.retryCount + "/" + networkAvatar.maxRetries + ")");
                            }
                        }
                    }

                    // avatar URL 变更时重置重试计数
                    Connections {
                        target: UserController
                        function onUserInfoChanged() {
                            networkAvatar.retryCount = 0;
                        }
                    }
                }
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            TapHandler {
                onTapped: avatarMenu.popup()
            }

            // 头像下拉菜单
            FluMenu {
                id: avatarMenu
                width: 180

                FluMenuItem {
                    text: UserController.username !== "" ? UserController.username : "用户"
                    enabled: false
                }
                FluMenuSeparator {}
                FluMenuItem {
                    text: "个人信息"
                    iconSource: FluentIcons.Contact
                    onClicked: console.log("[TopBar] 个人信息")
                }
                FluMenuItem {
                    text: "设置"
                    iconSource: FluentIcons.Settings
                    onClicked: console.log("[TopBar] 设置")
                }
                FluMenuSeparator {}
                FluMenuItem {
                    text: "退出登录"
                    iconSource: FluentIcons.SignOut
                    textColor: "#e74c3c"
                    onClicked: {
                        console.log("[TopBar] 退出登录");
                        LoginController.logout();
                    }
                }
            }
        }
    }

    // 底部阴影渐变
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.bottom
        height: 3
        z: 5

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#10000000"
            }
            GradientStop {
                position: 1.0
                color: "transparent"
            }
        }
    }
}
