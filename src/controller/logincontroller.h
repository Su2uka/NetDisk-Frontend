#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QObject>

class LoginController : public QObject
{
    Q_OBJECT
public:
    static LoginController* instance();

    Q_INVOKABLE void login(const QString &email, const QString &password, bool autoLogin);

    Q_INVOKABLE void getVerificationCode(const QString &email);

    Q_INVOKABLE void registerUser(const QString &email, const QString &code,
                                  const QString &password);

    // 检查自动登录状态
    Q_INVOKABLE void checkAutoLogin();

    // 退出登录
    Q_INVOKABLE void logout();

signals:
    void loginSuccess(const QString &token);
    void loginFailed(const QString &errorMessage);
    void autoLoginFailed(); // 自动登录失败 (token失效或不存在)
    void logoutSuccess();   // 退出登录成功
    
    void verificationCodeSent();
    void verificationCodeFailed(const QString &errMsg);

    void registerSuccess();
    void registerFailed(const QString &errMsg);

private:
    explicit LoginController(QObject *parent = nullptr);
};

#endif // LOGINCONTROLLER_H
