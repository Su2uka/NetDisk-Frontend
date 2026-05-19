#ifndef USERCONTROLLER_H
#define USERCONTROLLER_H

#include <QObject>

class UserController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int userId READ userId NOTIFY userInfoChanged)
    Q_PROPERTY(QString username READ username NOTIFY userInfoChanged)
    Q_PROPERTY(QString email READ email NOTIFY userInfoChanged)
    Q_PROPERTY(QString avatar READ avatar NOTIFY userInfoChanged)
    Q_PROPERTY(qint64 totalCapacity READ totalCapacity NOTIFY userInfoChanged)
    Q_PROPERTY(qint64 usedCapacity READ usedCapacity NOTIFY userInfoChanged)

public:
    static UserController* instance();

    int userId() const;
    QString username() const;
    QString email() const;
    QString avatar() const;
    qint64 totalCapacity() const;
    qint64 usedCapacity() const;

    /// 登录成功后调用，从后端拉取用户信息
    Q_INVOKABLE void fetchUserInfo();

signals:
    void userInfoChanged();

private:
    explicit UserController(QObject *parent = nullptr);

    int m_userId = 0;
    QString m_username;
    QString m_email;
    QString m_avatar;
    qint64 m_totalCapacity = 0;
    qint64 m_usedCapacity = 0;
};

#endif // USERCONTROLLER_H
