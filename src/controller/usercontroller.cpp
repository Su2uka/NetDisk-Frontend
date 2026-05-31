#include "usercontroller.h"
#include "../network/networkmanager.h"
#include <QJsonObject>
#include <QDebug>
#include <QSettings>

UserController* UserController::instance()
{
    static UserController inst;
    return &inst;
}

UserController::UserController(QObject *parent)
    : QObject(parent)
{
}

int UserController::userId() const { return m_userId; }
QString UserController::username() const { return m_username; }
QString UserController::email() const { return m_email; }
QString UserController::avatar() const { return m_avatar; }
qint64 UserController::totalCapacity() const { return m_totalCapacity; }
qint64 UserController::usedCapacity() const { return m_usedCapacity; }

void UserController::fetchUserInfo()
{
    NetworkManager::instance()->get("/users/me", {},
        [this](const QJsonObject &data) {
            m_userId        = data["id"].toInt();
            m_username      = data["username"].toString();
            m_email         = data["email"].toString();
            m_avatar        = data["avatar"].toString();
            m_totalCapacity = data["total_capacity"].toVariant().toLongLong();
            m_usedCapacity  = data["used_capacity"].toVariant().toLongLong();

            QSettings settings;
            settings.setValue("user/current_user_id", m_userId);

            qDebug() << "[User] 用户信息已更新:"
                     << m_username
                     << "已用:" << m_usedCapacity
                     << "总量:" << m_totalCapacity;

            emit userInfoChanged();
        },
        [](const QString &errMsg) {
            qWarning() << "[User] 获取用户信息失败:" << errMsg;
        }
    );
}
