import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import FluentUI

Item {
    id: root

    // 侧边栏宽度（含阴影）
    property int sidebarWidth: 200

    // 页面切换信号
    signal pageSelected(string url)

    // 暴露 navigationView 以便外部访问（如 push 页面）
    property alias navigationView: nav_view

    width: sidebarWidth + sidebarShadow.width

    //  Logo 区域
    Rectangle {
        id: logoHeader
        width: root.sidebarWidth
        height: 60
        color: "#ecedf0"
        z: 10

        Row {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 4
            spacing: 10

            Image {
                id: logoImage
                source: "qrc:/icon/res/icon/cloud-app-icon.svg"
                sourceSize: Qt.size(45, 45)
                width: 45
                height: 45
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                smooth: true
                anchors.verticalCenter: parent.verticalCenter
            }

            FluText {
                text: "AeroDrive"
                font.pixelSize: 18
                font.bold: true
                color: "#1a1a2e"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // ── 导航菜单项 ──
    NavItems {
        id: nav_items
        navigationView: nav_view
        onPageSelected: function (url) {
            root.pageSelected(url);
        }
    }

    // ── 导航视图 ──
    FluNavigationView {
        id: nav_view
        anchors.left: parent.left
        anchors.right: sidebarShadow.left
        anchors.top: logoHeader.bottom
        anchors.bottom: parent.bottom
        cellHeight: 60
        cellWidth: root.sidebarWidth

        buttonBack.visible: false
        hideNavAppBar: true
        navBackgroundColor: "#ecedf0"

        items: nav_items
        pageMode: FluNavigationViewType.NoStack
        displayMode: FluNavigationViewType.Open

        Component.onCompleted: {
            setCurrentIndex(0);
        }
    }

    // ── 右侧阴影渐变（替代硬分割线） ──
    Rectangle {
        id: sidebarShadow
        x: root.sidebarWidth
        y: 0
        width: 4
        height: parent.height
        z: 10

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: "#18000000"
            }
            GradientStop {
                position: 1.0
                color: "transparent"
            }
        }
    }
}
