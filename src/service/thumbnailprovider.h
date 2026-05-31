#ifndef THUMBNAILPROVIDER_H
#define THUMBNAILPROVIDER_H

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMutex>

/**
 * @brief 缩略图异步响应
 *
 * 每次 QML 请求 image://thumbnail/{fileId} 时创建一个实例。
 * 先查本地磁盘缓存，命中则直接返回；未命中则通过 HTTP
 * 向后端请求缩略图（带 Authorization），下载完成后写入缓存并返回。
 */
class ThumbnailResponse : public QQuickImageResponse
{
    Q_OBJECT
public:
    ThumbnailResponse(const QString &fileId,
                      const QSize &requestedSize,
                      const QString &token);

    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;

private:
    /// 返回本地缓存目录路径（自动创建）
    static QString cacheDir();
    /// 返回给定 fileId 的缓存文件完整路径
    static QString cachePath(const QString &fileId, const QString &extension = QStringLiteral("webp"));

    QImage  m_image;
    QString m_errorString;
    QSize   m_requestedSize;
};

/**
 * @brief 缩略图异步图片提供器（注册到 QML 引擎）
 *
 * 使用方式：
 *   engine.addImageProvider("thumbnail", new ThumbnailProvider);
 *
 * QML 中：
 *   Image { source: "image://thumbnail/" + model.fileId }
 */
class ThumbnailProvider : public QQuickAsyncImageProvider
{
public:
    ThumbnailProvider();

    QQuickImageResponse *requestImageResponse(
        const QString &id, const QSize &requestedSize) override;

};

#endif // THUMBNAILPROVIDER_H
