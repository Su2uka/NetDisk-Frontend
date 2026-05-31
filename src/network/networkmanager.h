#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <functional>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager* instance();

    // POST
    void post(const QString &apiPath, const QJsonObject &params,
              std::function<void(const QJsonObject&)> successCallback, 
              std::function<void(const QString&)> errorCallback);

    // POST（带额外 Header，如 X-Share-Token）
    void post(const QString &apiPath, const QJsonObject &params,
              const QMap<QString, QString> &extraHeaders,
              std::function<void(const QJsonObject&)> successCallback,
              std::function<void(const QString&)> errorCallback);
    // PUT
    void put(const QString &apiPath, const QJsonObject &params,
             std::function<void(const QJsonObject&)> successCallback,
             std::function<void(const QString&)> errorCallback);

    // GET
    void get(const QString &apiPath, const QJsonObject &params,
             std::function<void(const QJsonObject&)> successCallback,
             std::function<void(const QString&)> errorCallback);

    // GET（带额外 Header）
    void get(const QString &apiPath, const QJsonObject &params,
             const QMap<QString, QString> &extraHeaders,
             std::function<void(const QJsonObject&)> successCallback,
             std::function<void(const QString&)> errorCallback);

    // GET 返回数组的变体（如 /shares/files 返回 data: [...]）
    void getArray(const QString &apiPath, const QJsonObject &params,
                  const QMap<QString, QString> &extraHeaders,
                  std::function<void(const QJsonArray&)> successCallback,
                  std::function<void(const QString&)> errorCallback);

    // DELETE
    void deleteResource(const QString &apiPath,
                        std::function<void(const QJsonObject&)> successCallback,
                        std::function<void(const QString&)> errorCallback);

    // 设置全局 Token (登录成功后调用)
    void setToken(const QString &token);
    void setRefreshToken(const QString &token);
    QString token() const;

    // 辅助方法，处理401自动刷新并重试
    void handle401Error(std::function<void()> retryFunc, std::function<void(const QString&)> errorCallback);

    // 上传文件
    QNetworkReply* uploadFile(const QString &apiPath,
                              const QString &localPath,
                              const QString &targetFolderId,
                              const QString &fileHash,
                              std::function<void(qint64 sent, qint64 total)> progressCallback,
                              std::function<void(const QJsonObject&)> successCallback,
                              std::function<void(const QString&)> errorCallback);

    // PUT 二进制数据到预签名 URL（分片上传用）
    QNetworkReply* putRawData(const QUrl &presignedUrl,
                              const QByteArray &data,
                              std::function<void(qint64 sent, qint64 total)> progressCallback);

signals:
    void tokenInvalid(); // Token失效且刷新失败时发出

private:
    explicit NetworkManager(QObject *parent = nullptr);
    QNetworkAccessManager *m_manager;
    QString m_token;
    QString m_refreshToken;
    bool m_isRefreshing = false;
    QList<std::function<void()>> m_retryQueue;
};

#endif // NETWORKMANAGER_H
