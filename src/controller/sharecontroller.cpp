#include "sharecontroller.h"
#include "../network/networkmanager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QJsonObject>
#include <QDebug>

ShareController::ShareController(QObject *parent)
    : QObject{parent}
{
}

ShareController* ShareController::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

ShareController* ShareController::instance()
{
    static ShareController instance;
    return &instance;
}

void ShareController::createShare(const QString &fileId, int validDays, bool isPrivate)
{
    // 组装请求 JSON（参数名与后端接口对齐）
    QJsonObject params;
    params["file_id"] = fileId.toInt();
    params["expire_days"] = validDays;
    params["need_extract_code"] = isPrivate;

    // 发起 POST 请求到分享接口
    NetworkManager::instance()->post("/shares", params,
        [this](const QJsonObject &responseObj) {
            // 优先取 clipboard_text 字段
            QString clipboardText = responseObj["clipboard_text"].toString();

            // 兼容处理：如果后端没有直接返回 clipboard_text，则自行拼接
            if (clipboardText.isEmpty()) {
                QString shareUrl = responseObj["share_url"].toString();
                QString extractCode = responseObj["extract_code"].toString();
                if (!extractCode.isEmpty()) {
                    clipboardText = QString("链接：%1\n提取码：%2").arg(shareUrl, extractCode);
                } else {
                    clipboardText = QString("链接：%1").arg(shareUrl);
                }
            }

            onShareCreated(clipboardText);
        },
        [this](const QString &errorMsg) {
            emit showToast(QString("分享失败: %1").arg(errorMsg), true);
        });
}

void ShareController::onShareCreated(const QString &clipboardText)
{
    // 瞬间注入系统剪贴板
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(clipboardText);

    // 触发全局信号，让 UI 弹出 Toast
    emit showToast("分享口令已复制，快去发送给好友吧！", false);
}
