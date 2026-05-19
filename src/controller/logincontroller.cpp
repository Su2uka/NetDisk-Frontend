#include "logincontroller.h"
#include <QJsonObject>
#include <QSettings>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDebug>
#include "../global/AppConfig.h"
#include "../network/networkmanager.h"


LoginController::LoginController(QObject *parent)
    : QObject{parent}
{
    connect(NetworkManager::instance(), &NetworkManager::tokenInvalid, this, [this]() {
        emit autoLoginFailed();
    });
}

void LoginController::login(const QString &email, const QString &password, bool autoLogin)
{
    // 1. 构建 OAuth2 规范要求的表单数据 (x-www-form-urlencoded)
    QUrlQuery query;
    query.addQueryItem("username", email);
    query.addQueryItem("password", password);
    QByteArray data = query.toString(QUrl::FullyEncoded).toUtf8();

    // 2. 构建请求
    QUrl url(AppConfig::API_BASE + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // 3. 发送请求
    QNetworkAccessManager* m_manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = m_manager->post(request, data);

    // 4. 处理异步响应
    connect(reply, &QNetworkReply::finished, this, [this, reply, autoLogin]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        // 处理错误状态 (如 HTTP 400/401)
        if (reply->error() != QNetworkReply::NoError) {
            // 提取 FastAPI 抛出的标准 detail 错误字段
            QString errMsg = jsonObj.contains("detail") ? jsonObj["detail"].toString() : "登录失败，请检查网络或账号密码";
            emit loginFailed(errMsg);
            return;
        }

        // 提取成功后的平铺 Token
        if (jsonObj.contains("access_token")) {
            QString token = jsonObj["access_token"].toString();
            QString refreshToken = jsonObj.contains("refresh_token") ? jsonObj["refresh_token"].toString() : "";

            // 设置 NetworkManager Token (内存中)
            NetworkManager::instance()->setToken(token);
            if (!refreshToken.isEmpty()) {
                NetworkManager::instance()->setRefreshToken(refreshToken);
            }

            // 根据 autoLogin 保存或清除 Token
            QSettings settings;
            if (autoLogin) {
                settings.setValue("user/access_token", token);
                settings.setValue("user/token", token); // fallback compatibility
                if (!refreshToken.isEmpty()) {
                    settings.setValue("user/refresh_token", refreshToken);
                }
                qDebug("设置token");
            } else {
                settings.remove("user/token");
                settings.remove("user/access_token");
                settings.remove("user/refresh_token");
                qDebug("去除token");
            }

            emit loginSuccess(token);
        } else {
            emit loginFailed("返回格式错误：未获取到 access_token");
        }
    });
}

void LoginController::getVerificationCode(const QString &email)
{
    // 构造请求参数
    QJsonObject params;
    params["email"] = email;

    // 发送获取验证码请求
    NetworkManager::instance()->post("/auth/send-code", params,
        [this](const QJsonObject &) {
            // 请求成功
            emit verificationCodeSent();
        },
        [this](const QString &errMsg) {
            // 请求失败
            emit verificationCodeFailed(errMsg);
        }
    );
}

void LoginController::registerUser(const QString &email, const QString &code,
                                   const QString &password)
{
    QJsonObject params;
    params["email"] = email;
    params["code"] = code;
    params["password"] = password;

    NetworkManager::instance()->post("/auth/register", params,
        [this](const QJsonObject &) {
            emit registerSuccess();
        },
        [this](const QString &errMsg) {
            emit registerFailed(errMsg);
        }
    );
}

void LoginController::checkAutoLogin()
{
    QSettings settings;
    QString savedToken = settings.value("user/access_token").toString();
    if (savedToken.isEmpty()) {
        savedToken = settings.value("user/token").toString();
    }
    QString savedRefreshToken = settings.value("user/refresh_token").toString();

    if (savedToken.isEmpty()) {
        qDebug("Token为空");
        emit autoLoginFailed();
        return;
    }

    // 设置内存中的临时 Token
    NetworkManager::instance()->setToken(savedToken);
    if (!savedRefreshToken.isEmpty()) {
        NetworkManager::instance()->setRefreshToken(savedRefreshToken);
    }

    // 后端验证 Token GET /api/v1/users/me
    NetworkManager::instance()->get("/users/me", {},
        // 成功: Token 有效
        [this, savedToken](const QJsonObject &) {
            emit loginSuccess(savedToken); 
        },
        // 失败: Token 无效或过期，以及断网等情况
        [this](const QString &) {
            QSettings settings;
            settings.remove("user/token"); // 清除无效 Token
            settings.remove("user/access_token");
            settings.remove("user/refresh_token");
            emit autoLoginFailed();
        }
    );
}

void LoginController::logout()
{
    QSettings settings;
    QString refreshToken = settings.value("user/refresh_token").toString();
    
    if (!refreshToken.isEmpty()) {
        QJsonObject params;
        params["refresh_token"] = refreshToken;
        NetworkManager::instance()->post("/auth/logout", params, [](const QJsonObject&){}, [](const QString&){});
    }

    settings.remove("user/token");
    settings.remove("user/access_token");
    settings.remove("user/refresh_token");
    NetworkManager::instance()->setToken("");
    NetworkManager::instance()->setRefreshToken("");

    emit logoutSuccess(); 
}
