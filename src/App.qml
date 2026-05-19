import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FluentUI 1.0

FluLauncher {
    id: app
    Component.onCompleted: {
        FluApp.init(app);
        FluApp.windowIcon = "qrc:/icon/res/icon/cloud-app-icon.ico";
        FluRouter.routes = {
            "/": "qrc:/main.qml",
            "/login": "qrc:/qml/Auth/LoginPage.qml",
            "/splash": "qrc:/qml/Splash/SplashPage.qml",
            "/term": "qrc:/qml/Auth/ServiceTermsPage.qml",
            "/main": "qrc:/qml/Main/MainPage.qml"
        };

        // 启动时跳转到 Splash 页面
        FluRouter.navigate("/splash");
    }
}
