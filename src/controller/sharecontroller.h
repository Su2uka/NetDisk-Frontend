#ifndef SHARECONTROLLER_H
#define SHARECONTROLLER_H

#include <QObject>
#include <QtQml>

/**
 * @brief 文件分享控制器
 * 负责处理前端的发起分享请求，并通过 NetworkManager 调用后端接口
 */
class ShareController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ShareController(QObject *parent = nullptr);
    static ShareController* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static ShareController* instance();

    /**
     * @brief 发起创建分享链接请求
     * @param fileId 文件ID
     * @param validDays 有效期（0或-1表示永久，7表示7天，1表示1天）
     * @param isPrivate 是否私密分享（需要提取码）
     */
    Q_INVOKABLE void createShare(const QString &fileId, int validDays, bool isPrivate);

signals:
    /**
     * @brief 触发全局 Toast 提示信号
     * @param message 提示内容
     * @param isError 是否为错误提示
     */
    void showToast(const QString &message, bool isError = false);

private:
    /**
     * @brief 分享创建成功的回调处理
     * @param clipboardText 需要写入剪贴板的分享口令文本
     */
    void onShareCreated(const QString &clipboardText);
};

#endif // SHARECONTROLLER_H
