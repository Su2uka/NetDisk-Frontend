#include "thumbnailprovider.h"
#include "../network/networkmanager.h"
#include "userstoragepaths.h"
#include "../global/AppConfig.h"

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QQuickTextureFactory>
#include <QNetworkRequest>
#include <QDebug>
#include <QTimer>
#include <QStringList>

// ════════════════════════════════════════════
//  ThumbnailResponse
// ════════════════════════════════════════════

ThumbnailResponse::ThumbnailResponse(const QString &fileId,
                                     const QSize &requestedSize,
                                     const QString &token)
    : m_requestedSize(requestedSize)
{
    // 1. 尝试从本地磁盘缓存读取
    const QStringList cachedPaths = {
        cachePath(fileId, "webp"),
        cachePath(fileId, "jpg")
    };
    for (const QString &localPath : cachedPaths) {
        if (!QFileInfo::exists(localPath))
            continue;
        m_image.load(localPath);
        if (!m_image.isNull()) {
            // 如果 requestedSize 有效，则缩放
            if (m_requestedSize.isValid() && !m_requestedSize.isEmpty()) {
                m_image = m_image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            QTimer::singleShot(0, this, &ThumbnailResponse::finished);
            return;
        }
    }

    // 2. 本地缓存未命中 → 向后端请求缩略图
    QString url = AppConfig::API_BASE + "/files/" + fileId + "/thumbnail?size=200";

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    auto *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(request);

    // 关键：连接 finished 信号到槽函数
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileId]() {
        reply->deleteLater();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            if (statusCode == 404 || statusCode == 410) {
                qDebug() << "[缩略图] 无可用缩略图:" << fileId << "status:" << statusCode;
                emit finished();
                return;
            }

            m_errorString = reply->errorString();
            qDebug() << "[缩略图] 下载失败:" << fileId << m_errorString;
            emit finished();
            return;
        }

        QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            if (statusCode == 204) {
                qDebug() << "[缩略图] 无可用缩略图:" << fileId << "status:" << statusCode;
                emit finished();
                return;
            }

            m_errorString = "Empty response";
            emit finished();
            return;
        }

        // 解析图片数据
        if (!m_image.loadFromData(data)) {
            m_errorString = "Failed to parse image data";
            qDebug() << "[缩略图] 解析失败:" << fileId;
            emit finished();
            return;
        }

        // 写入本地缓存（异步写入，不阻塞返回）
        QString localPath = cachePath(fileId);
        QDir().mkpath(cacheDir());

        // 根据后端返回的 Content-Type 决定缓存格式
        // 默认保存为 WebP，如果不支持则保存为 JPEG
        if (!m_image.save(localPath, "WEBP", 85)) {
            // WebP 不可用时降级为 JPEG
            localPath = cacheDir() + "/" + fileId + ".jpg";
            m_image.save(localPath, "JPEG", 85);
        }

        // 按请求尺寸缩放
        if (m_requestedSize.isValid() && !m_requestedSize.isEmpty()) {
            m_image = m_image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        qDebug() << "[缩略图] 下载并缓存成功:" << fileId;
        emit finished();
    });
}

QQuickTextureFactory *ThumbnailResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

QString ThumbnailResponse::errorString() const
{
    return m_errorString;
}

// ── 缓存工具函数 ──

QString ThumbnailResponse::cacheDir()
{
    return UserStoragePaths::userCacheDir("thumbnails");
}

QString ThumbnailResponse::cachePath(const QString &fileId, const QString &extension)
{
    return cacheDir() + "/" + fileId + "." + extension;
}

// ════════════════════════════════════════════
//  ThumbnailProvider
// ════════════════════════════════════════════

ThumbnailProvider::ThumbnailProvider()
{
    // ThumbnailProvider 的生命周期由 QML 引擎管理。
}

QQuickImageResponse *ThumbnailProvider::requestImageResponse(
    const QString &id, const QSize &requestedSize)
{
    // id 的格式为 "{fileId}"
    // 从 NetworkManager 获取当前 token
    // 注意：NetworkManager 的 m_token 是私有的，我们需要一个 getter
    // 这里通过读取 NetworkManager 单例获取 token
    QString token;

    // 通过反射或友元方式获取 token 不理想
    // 最佳方案：在 NetworkManager 中增加一个 public getter
    // 暂时通过直接获取实现（需要在 NetworkManager 中添加 token() 方法）
    token = NetworkManager::instance()->token();

    return new ThumbnailResponse(id, requestedSize, token);
}
