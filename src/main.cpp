#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml>
#include <QSettings>
#include "controller/logincontroller.h"
#include "controller/filecontroller.h"
#include "controller/uploadcontroller.h"
#include "controller/downloadcontroller.h"
#include "controller/recyclebincontroller.h"
#include "controller/usercontroller.h"
#include "controller/sharecontroller.h"
#include "controller/previewcontroller.h"
#include "controller/clipboardcontroller.h"
#include "controller/mysharecontroller.h"
#include "controller/favoritescontroller.h"
#include "controller/settingscontroller.h"
#include "service/transferhistorymanager.h"
#include "service/recentfilesmanager.h"
#include "service/thumbnailprovider.h"
#include "global/AppConfig.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QQuickStyle::setStyle("Basic");

    // 允许 QML 中使用 XMLHttpRequest 读取本地文件 (如 qrc 资源)
    qputenv("QML_XHR_ALLOW_FILE_READ", "1");

    // 配置应用程序信息
    QCoreApplication::setOrganizationName(AppConfig::ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(AppConfig::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(AppConfig::APPLICATION_NAME);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml/ThirdParty/QtMediaPlayerDemo"));
    // ── 声明式注册：用 qmlRegisterSingletonInstance 替代 setContextProperty ──
    // 好处：QML 工具链可分析类型、更清晰的模块边界、更好的性能
    const char *uri = "App";

    auto *loginController = new LoginController(&app);
    qmlRegisterSingletonInstance(uri, 1, 0, "LoginController", loginController);

    auto *fileController = new FileController(&app);
    qmlRegisterSingletonInstance(uri, 1, 0, "FileController", fileController);

    qmlRegisterSingletonInstance(uri, 1, 0, "UploadController", UploadController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "DownloadController", DownloadController::instance());

    auto *historyMgr = TransferHistoryManager::instance();
    historyMgr->connectControllers();
    qmlRegisterSingletonInstance(uri, 1, 0, "TransferHistory", historyMgr);

    auto *recentFilesMgr = RecentFilesManager::instance();
    recentFilesMgr->connectControllers();
    qmlRegisterSingletonInstance(uri, 1, 0, "RecentFilesManager", recentFilesMgr);

    qmlRegisterSingletonInstance(uri, 1, 0, "RecycleBin", RecycleBinController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "UserController", UserController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "ShareController", ShareController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "PreviewController", new PreviewController(&app));
    qmlRegisterSingletonInstance(uri, 1, 0, "ClipboardController", ClipboardController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "MyShareController", MyShareController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "FavoritesController", FavoritesController::instance());
    qmlRegisterSingletonInstance(uri, 1, 0, "SettingsController", SettingsController::instance());

    // ── 缩略图图片提供器 ──
    engine.addImageProvider("thumbnail", new ThumbnailProvider);

    const QUrl url(QStringLiteral("qrc:/App.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
