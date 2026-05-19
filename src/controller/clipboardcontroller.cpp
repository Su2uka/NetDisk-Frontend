#include "clipboardcontroller.h"
#include "usercontroller.h"
#include "../network/networkmanager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

// ════════════════════════════════════════════
//  单例
// ════════════════════════════════════════════

ClipboardController::ClipboardController(QObject *parent)
    : QObject{parent}
{
    // 监听应用程序的窗口状态变化（后台 ↔ 前台）
    // 注意：信号在构造时就连接，但 onApplicationStateChanged 内部会检查 m_enabled
    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &ClipboardController::onApplicationStateChanged);

}

ClipboardController* ClipboardController::instance()
{
    static ClipboardController inst;
    return &inst;
}

// ════════════════════════════════════════════
//  启用 / 停用监听（由 QML 主页控制）
// ════════════════════════════════════════════

void ClipboardController::startMonitoring()
{
    m_enabled = true;
    m_lastDetectedKey.clear();
    qDebug() << "[剪贴板嗅探] 监听已启用";
}

void ClipboardController::stopMonitoring()
{
    m_enabled = false;
    qDebug() << "[剪贴板嗅探] 监听已停用";
}

// ════════════════════════════════════════════
//  窗口激活时自动检测剪贴板
// ════════════════════════════════════════════

void ClipboardController::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (!m_enabled)
        return;

    if (state != Qt::ApplicationActive)
        return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text().trimmed();

    if (text.isEmpty())
        return;

    QString shareKey, extractCode;
    if (!parseShareText(text, shareKey, extractCode))
        return;

    if (shareKey == m_lastDetectedKey)
        return;

    m_lastDetectedKey = shareKey;

    qDebug() << "[剪贴板嗅探] 检测到分享口令:" << shareKey
             << "提取码:" << (extractCode.isEmpty() ? "(无)" : extractCode);

    emit shareCodeDetected(shareKey, extractCode);
}

// ════════════════════════════════════════════
//  正则解析口令文本
// ════════════════════════════════════════════

bool ClipboardController::parseShareText(const QString &text,
                                          QString &outShareKey,
                                          QString &outExtractCode)
{
    // 匹配格式：【轻云盘】发来分享...口令：8273d5，提取码：lr2k
    static QRegularExpression reShareKey(
        QStringLiteral("口令[：:]\\s*([A-Za-z0-9]{4,12})")
    );

    static QRegularExpression reExtractCode(
        QStringLiteral("提取码[：:]\\s*([A-Za-z0-9]{2,8})")
    );

    QRegularExpressionMatch matchKey = reShareKey.match(text);
    if (!matchKey.hasMatch())
        return false;

    outShareKey = matchKey.captured(1);

    QRegularExpressionMatch matchCode = reExtractCode.match(text);
    outExtractCode = matchCode.hasMatch() ? matchCode.captured(1) : QString();

    return true;
}

// ════════════════════════════════════════════
//  辅助
// ════════════════════════════════════════════

void ClipboardController::setLoading(bool loading)
{
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

// ════════════════════════════════════════════
//  用户操作：验证口令
// ════════════════════════════════════════════

void ClipboardController::openShare(const QString &shareKey, const QString &extractCode)
{
    qDebug() << "[剪贴板嗅探] 验证口令:" << shareKey;
    setLoading(true);

    QJsonObject body;
    if (!extractCode.isEmpty()) {
        body["extract_code"] = extractCode;
    }

    QString apiPath = "/shares/" + shareKey + "/verify";

    NetworkManager::instance()->post(
        apiPath, body,
        [this, shareKey](const QJsonObject &data) {
            setLoading(false);

            m_shareToken = data["share_token"].toString();
            m_currentShareKey = shareKey;

            QJsonObject sharerInfo = data["sharer_info"].toObject();
            QString sharerName = sharerInfo["nickname"].toString();
            QString sharerAvatar = sharerInfo["avatar"].toString();
            int sharerId = sharerInfo["id"].toInt();

            QJsonObject rootFile = data["root_file"].toObject();
            int fileId = rootFile["file_id"].toInt();
            QString fileName = rootFile["name"].toString();
            bool isFolder = rootFile["is_folder"].toBool();

            qDebug() << "[剪贴板嗅探] 验证成功:"
                     << "分享者=" << sharerName
                     << "分享者ID=" << sharerId
                     << "文件=" << fileName;

            // 检测是否是自己的分享
            int currentUserId = UserController::instance()->userId();
            if (currentUserId > 0 && sharerId == currentUserId) {
                qDebug() << "[剪贴板嗅探] 检测到自己的分享，跳过";
                emit ownShareDetected();
                return;
            }

            emit shareVerified(m_shareToken, sharerName, sharerAvatar, fileName, fileId, isFolder);
        },
        [this](const QString &errMsg) {
            setLoading(false);
            qWarning() << "[剪贴板嗅探] 验证失败:" << errMsg;
            emit shareVerifyFailed(errMsg);
        }
    );
}

// ════════════════════════════════════════════
//  用户操作：忽略
// ════════════════════════════════════════════

void ClipboardController::dismissShare(const QString &shareKey)
{
    m_lastDetectedKey = shareKey;
    qDebug() << "[剪贴板嗅探] 用户忽略口令:" << shareKey;
}

// ════════════════════════════════════════════
//  加载分享文件列表（GET /shares/files）
// ════════════════════════════════════════════

void ClipboardController::loadShareFiles(int parentId)
{
    if (m_shareToken.isEmpty()) {
        qWarning() << "[剪贴板嗅探] 无法加载分享文件：shareToken 为空";
        return;
    }

    setLoading(true);

    QJsonObject params;
    // parentId == -1 表示请求根级（不传 parent_id），>= 0 则传
    if (parentId >= 0) {
        params["parent_id"] = parentId;
    }

    QMap<QString, QString> headers;
    headers["X-Share-Token"] = m_shareToken;

    // /shares/files 返回的 data 是数组，使用 getArray
    NetworkManager::instance()->getArray(
        "/shares/files", params, headers,
        [this, parentId](const QJsonArray &arr) {
            setLoading(false);

            QVariantList children;
            for (const QJsonValue &val : arr) {
                QJsonObject obj = val.toObject();
                QVariantMap file;
                file["fileId"]   = obj["file_id"].toInt();
                file["name"]     = obj["name"].toString();
                file["isFolder"] = obj["is_folder"].toBool();
                file["size"]     = obj["size"].toVariant().toLongLong();
                children.append(file);
            }

            qDebug() << "[剪贴板嗅探] 分享文件子节点加载完成: parentId="
                     << parentId << " 数量=" << children.size();

            emit shareChildrenLoaded(parentId, children);
        },
        [this](const QString &errMsg) {
            setLoading(false);
            qWarning() << "[剪贴板嗅探] 加载分享文件失败:" << errMsg;
        }
    );
}

// ════════════════════════════════════════════
//  加载用户自己网盘的文件夹列表
// ════════════════════════════════════════════

void ClipboardController::loadMyFolders(int parentId)
{
    setLoading(true);

    QJsonObject params;
    //根目录（parentId == 0）不传 parent_id
    if (parentId > 0) {
        params["parent_id"] = QString::number(parentId);
    }

    NetworkManager::instance()->get(
        "/files/list", params,
        [this, parentId](const QJsonObject &data) {
            setLoading(false);

            QVariantList children;

            QJsonArray fileArray = data["files"].toArray();
            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();
                // 只取文件夹
                if (!obj["is_folder"].toBool())
                    continue;

                QVariantMap folder;
                folder["fileId"] = obj["id"].toVariant().toInt();
                folder["name"]   = obj["name"].toString();
                children.append(folder);
            }

            qDebug() << "[剪贴板嗅探] 文件夹子节点加载完成: parentId="
                     << parentId << " 数量=" << children.size();

            emit folderChildrenLoaded(parentId, children);
        },
        [this](const QString &errMsg) {
            setLoading(false);
            qWarning() << "[剪贴板嗅探] 加载网盘文件夹失败:" << errMsg;
        }
    );
}

// ════════════════════════════════════════════
//  保存到网盘（POST /shares/save）
// ════════════════════════════════════════════

void ClipboardController::saveToMyDisk(const QVariantList &sourceFileIds, int targetParentId)
{
    if (m_shareToken.isEmpty()) {
        emit saveFailed("分享令牌无效，请重新验证口令");
        return;
    }

    setLoading(true);

    QJsonArray idsArray;
    for (const QVariant &v : sourceFileIds) {
        idsArray.append(v.toInt());
    }

    QJsonObject body;
    body["source_file_ids"] = idsArray;
    body["target_parent_id"] = targetParentId;

    QMap<QString, QString> headers;
    headers["X-Share-Token"] = m_shareToken;

    NetworkManager::instance()->post(
        "/shares/save", body, headers,
        [this](const QJsonObject &data) {
            setLoading(false);
            QString msg = data.contains("message") ? data["message"].toString() : "保存成功";
            qDebug() << "[剪贴板嗅探] 保存成功:" << msg;
            emit saveSuccess(msg);
        },
        [this](const QString &errMsg) {
            setLoading(false);
            qWarning() << "[剪贴板嗅探] 保存失败:" << errMsg;
            emit saveFailed(errMsg);
        }
    );
}
