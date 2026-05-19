import QtQuick
import FluentUI

FluObject {
    id: nav_model

    property var navigationView
    // 页面切换回调，由 Sidebar 注入
    property var onPageSelected: function (url) {}

    // 首页
    FluPaneItem {
        title: "首页"
        icon: FluentIcons.Home
        iconColor: "#25262b"
        textColor: "#25262b"
        url: "qrc:/qml/View/Home/HomePage.qml"
        onTap: {
            nav_model.onPageSelected(url);
        }
    }

    // 文件
    FluPaneItem {
        title: "文件"
        icon: FluentIcons.FileExplorer
        iconColor: "#25262b"
        textColor: "#25262b"
        url: "qrc:/qml/View/File/FilePage.qml"
        onTap: {
            nav_model.onPageSelected(url);
        }
    }

    // 传输
    FluPaneItem {
        title: "传输"
        iconDelegate: Component {
            Image {
                source: "qrc:/icon/res/icon/transmission.svg"
                width: 28
                height: 28
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(30, 30)
            }
        }
        iconColor: "#25262b"
        textColor: "#25262b"
        url: "qrc:/qml/View/Transfer/TransferPage.qml"
        onTap: {
            nav_model.onPageSelected(url);
        }
    }

    // 回收站
    FluPaneItem {
        title: "回收站"
        icon: FluentIcons.Delete
        iconColor: "#25262b"
        textColor: "#25262b"
        url: "qrc:/qml/View/Home/HomePage.qml"
        onTap: {
            nav_model.onPageSelected(url);
        }
    }
}
