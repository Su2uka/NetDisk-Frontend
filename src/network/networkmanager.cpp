#include "networkmanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include "../global/AppConfig.h"

NetworkManager::NetworkManager(QObject *parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
}

NetworkManager* NetworkManager::instance()
{
    static NetworkManager instance;
    return &instance;
}

void NetworkManager::setToken(const QString &token)
{
    m_token = token;
}

void NetworkManager::setRefreshToken(const QString &token)
{
    m_refreshToken = token;
}

QString NetworkManager::token() const
{
    return m_token;
}

void NetworkManager::handle401Error(std::function<void()> retryFunc, std::function<void(const QString&)> errorCallback)
{
    if (m_isRefreshing) {
        m_retryQueue.append(retryFunc);
        return;
    }

    if (m_refreshToken.isEmpty()) {
        emit tokenInvalid();
        if (errorCallback) errorCallback("登录已过期，请重新登录");
        return;
    }

    m_isRefreshing = true;
    m_retryQueue.append(retryFunc);

    QJsonObject params;
    params["refresh_token"] = m_refreshToken;

    QUrl url(AppConfig::API_BASE + "/auth/refresh");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = QJsonDocument(params).toJson();
    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, errorCallback]() {
        reply->deleteLater();
        m_isRefreshing = false;

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError && statusCode == 200 && jsonObj.contains("access_token")) {
            m_token = jsonObj["access_token"].toString();
            if (jsonObj.contains("refresh_token")) {
                m_refreshToken = jsonObj["refresh_token"].toString();
            }

            QSettings settings;
            settings.setValue("user/access_token", m_token);
            settings.setValue("user/refresh_token", m_refreshToken);

            auto queue = m_retryQueue;
            m_retryQueue.clear();
            for (auto &func : queue) {
                func();
            }
        } else {
            m_retryQueue.clear();
            m_token = "";
            m_refreshToken = "";
            QSettings settings;
            settings.remove("user/access_token");
            settings.remove("user/refresh_token");
            settings.remove("user/token");
            emit tokenInvalid();
            if (errorCallback) errorCallback("登录状态已过期，请重新登录");
        }
    });
}

void NetworkManager::put(const QString &apiPath, const QJsonObject &params,
                             std::function<void(const QJsonObject&)> successCallback,
                             std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QByteArray data = QJsonDocument(params).toJson();
    QNetworkReply *reply = m_manager->put(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, successCallback, errorCallback, reply]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 401 && apiPath != "/auth/refresh") {
            handle401Error([this, apiPath, params, successCallback, errorCallback]() {
                put(apiPath, params, successCallback, errorCallback);
            }, errorCallback);
            return;
        }

        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }

        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                successCallback(jsonObj["data"].toObject());
            } else {
                errorCallback(jsonObj["msg"].toString());
            }
        } else {
            successCallback(jsonObj);
        }
    });
}

void NetworkManager::post(const QString &apiPath, const QJsonObject &params,
                              std::function<void(const QJsonObject&)> successCallback,
                              std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QByteArray data = QJsonDocument(params).toJson();
    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, successCallback, errorCallback, reply]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        // 1. 优先判断 HTTP 状态码 (FastAPI 返回 400/401 等会自动触发 NetworkError)
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        if (statusCode == 401 && apiPath != "/auth/refresh") {
            handle401Error([this, apiPath, params, successCallback, errorCallback]() {
                post(apiPath, params, successCallback, errorCallback);
            }, errorCallback);
            return;
        }

        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            // 尝试提取 FastAPI 标准的 "detail" 错误信息
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }

        // 2. 处理 HTTP 200 成功响应
        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                // 如果后端返回了 data，可以直接把 data 传出去，方便上层解析
                successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                // 适配自定义的 "message" 字段
                QString msg = jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败";
                errorCallback(msg);
            }
        } else {
            //如果后端没按规范返回 code，但也成功了 (HTTP 200)
            successCallback(jsonObj);
        }
    });
}


// POST（带额外自定义 Header）
void NetworkManager::post(const QString &apiPath, const QJsonObject &params,
                          const QMap<QString, QString> &extraHeaders,
                          std::function<void(const QJsonObject&)> successCallback,
                          std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    for (auto it = extraHeaders.constBegin(); it != extraHeaders.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QByteArray data = QJsonDocument(params).toJson();
    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, extraHeaders, successCallback, errorCallback, reply]() {
        reply->deleteLater();
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401 && apiPath != "/auth/refresh") {
            handle401Error([this, apiPath, params, extraHeaders, successCallback, errorCallback]() {
                post(apiPath, params, extraHeaders, successCallback, errorCallback);
            }, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }
        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                errorCallback(jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败");
            }
        } else {
            successCallback(jsonObj);
        }
    });
}

// GET（带额外自定义 Header）
void NetworkManager::get(const QString &apiPath, const QJsonObject &params,
                         const QMap<QString, QString> &extraHeaders,
                         std::function<void(const QJsonObject&)> successCallback,
                         std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            query.addQueryItem(it.key(), it.value().toVariant().toString());
        }
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    for (auto it = extraHeaders.constBegin(); it != extraHeaders.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, extraHeaders, successCallback, errorCallback, reply]() {
        reply->deleteLater();
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            handle401Error([this, apiPath, params, extraHeaders, successCallback, errorCallback]() {
                get(apiPath, params, extraHeaders, successCallback, errorCallback);
            }, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }
        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                errorCallback(jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败");
            }
        } else {
            successCallback(jsonObj);
        }
    });
}

// GET 返回数组的变体（data 字段为 JSON 数组）
void NetworkManager::getArray(const QString &apiPath, const QJsonObject &params,
                              const QMap<QString, QString> &extraHeaders,
                              std::function<void(const QJsonArray&)> successCallback,
                              std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            query.addQueryItem(it.key(), it.value().toVariant().toString());
        }
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }
    for (auto it = extraHeaders.constBegin(); it != extraHeaders.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, extraHeaders, successCallback, errorCallback, reply]() {
        reply->deleteLater();
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            handle401Error([this, apiPath, params, extraHeaders, successCallback, errorCallback]() {
                getArray(apiPath, params, extraHeaders, successCallback, errorCallback);
            }, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }
        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                // 关键区别：取 data 字段为数组
                successCallback(jsonObj["data"].toArray());
            } else {
                errorCallback(jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败");
            }
        } else {
            successCallback(jsonDoc.array());
        }
    });
}


void NetworkManager::get(const QString &apiPath, const QJsonObject &params,
                         std::function<void(const QJsonObject&)> successCallback,
                         std::function<void(const QString&)> errorCallback)
{
    // 拼接基础 URL
    QUrl url(AppConfig::API_BASE + apiPath);

    // 如果有 GET 参数，将其转化为 URL 查询字符串 (Query String)
    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            // 将 QJsonObject 里的键值对转为字符串形式
            query.addQueryItem(it.key(), it.value().toVariant().toString());
        }
        url.setQuery(query);
    }

    QNetworkRequest request(url);

    // 设置通用 Header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    // 如果有 Token，自动添加到 Header
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    // 发送 GET 请求
    QNetworkReply *reply = m_manager->get(request);

    // 4. 处理响应
    connect(reply, &QNetworkReply::finished, this, [this, apiPath, params, successCallback, errorCallback, reply]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        // 优先判断 HTTP 状态码 (FastAPI 返回 400/401 等会自动触发 NetworkError)
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            handle401Error([this, apiPath, params, successCallback, errorCallback]() {
                get(apiPath, params, successCallback, errorCallback);
            }, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            // 尝试提取 FastAPI 标准的 "detail" 错误信息
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            errorCallback(errMsg);
            return;
        }

        // 处理 HTTP 200 成功响应
        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                // 如果后端返回了 data，可以直接把 data 传出去，方便上层解析
                successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                // 适配自定义的 "message" 字段
                QString msg = jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败";
                errorCallback(msg);
            }
        } else {
            // 兜底：如果后端没按规范返回 code，但也成功了 (HTTP 200)
            successCallback(jsonObj);
        }
    });
}


void NetworkManager::deleteResource(const QString &apiPath,
                                     std::function<void(const QJsonObject&)> successCallback,
                                     std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QNetworkReply *reply = m_manager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, apiPath, successCallback, errorCallback, reply]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            handle401Error([this, apiPath, successCallback, errorCallback]() {
                deleteResource(apiPath, successCallback, errorCallback);
            }, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            if (errorCallback) errorCallback(errMsg);
            return;
        }

        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                if (successCallback) successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                QString msg = jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败";
                if (errorCallback) errorCallback(msg);
            }
        } else {
            if (successCallback) successCallback(jsonObj);
        }
    });
}


QNetworkReply* NetworkManager::uploadFile(const QString &apiPath,
                                          const QString &localPath,
                                          const QString &targetFolderId,
                                          const QString &fileHash,
                                          std::function<void(qint64, qint64)> progressCallback,
                                          std::function<void(const QJsonObject&)> successCallback,
                                          std::function<void(const QString&)> errorCallback)
{
    QUrl url(AppConfig::API_BASE + apiPath);
    QNetworkRequest request(url);

    // 超时设置：对于大文件，如果中途断网卡住，此超时可强制断开（它是指没有任何数据传输的空闲超时）
    request.setTransferTimeout(AppConfig::NETWORK_TIMEOUT);

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 1. 附加目标目录 ID
    QHttpPart parentIdPart;
    parentIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"parent_id\""));
    parentIdPart.setBody(targetFolderId.toUtf8());
    multiPart->append(parentIdPart);

    // 2. 附加文件哈希
    QHttpPart hashPart;
    hashPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file_hash\""));
    hashPart.setBody(fileHash.toUtf8());
    multiPart->append(hashPart);

    // 3. 附加文件流
    QHttpPart filePart;
    QFileInfo fileInfo(localPath);
    QString header = QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName());
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(header));

    QFile *file = new QFile(localPath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        delete multiPart;
        if (errorCallback) errorCallback("无法打开本地文件: " + localPath);
        return nullptr;
    }

    filePart.setBodyDevice(file);
    file->setParent(multiPart); // 让 QFile 随 multiPart 一起被销毁
    multiPart->append(filePart);

    // 发起网络请求
    QNetworkReply *reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply); // 让 multiPart 随 reply 一起被销毁

    // 连接进度回调
    if (progressCallback) {
        connect(reply, &QNetworkReply::uploadProgress, this, [progressCallback](qint64 sent, qint64 total) {
            if (total > 0) {
                progressCallback(sent, total);
            }
        });
    }

    // 连接完成回调
    connect(reply, &QNetworkReply::finished, this, [this, apiPath, localPath, targetFolderId, fileHash, progressCallback, successCallback, errorCallback, reply]() {
        reply->deleteLater();

        // 拦截用户主动取消操作
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            if (errorCallback) errorCallback("已取消");
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            // For upload file, we only trigger refresh but don't retry, let user retry manually
            handle401Error([](){}, errorCallback);
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : reply->errorString();
            if (errorCallback) errorCallback(errMsg);
            return;
        }

        if (jsonObj.contains("code")) {
            if (jsonObj["code"].toInt() == 200) {
                if (successCallback) successCallback(jsonObj.contains("data") ? jsonObj["data"].toObject() : jsonObj);
            } else {
                QString msg = jsonObj.contains("message") ? jsonObj["message"].toString() : "业务处理失败";
                if (errorCallback) errorCallback(msg);
            }
        } else {
            if (successCallback) successCallback(jsonObj);
        }
    });

    return reply;
}

QNetworkReply* NetworkManager::putRawData(const QUrl &presignedUrl,
                                           const QByteArray &data,
                                           std::function<void(qint64, qint64)> progressCallback)
{
    QNetworkRequest request(presignedUrl);
    // 预签名 URL 不需要 Authorization，直接发送二进制数据
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());

    QNetworkReply *reply = m_manager->put(request, data);

    if (progressCallback) {
        connect(reply, &QNetworkReply::uploadProgress, this,
                [progressCallback](qint64 sent, qint64 total) {
            progressCallback(sent, total);
        });
    }

    return reply;
}
