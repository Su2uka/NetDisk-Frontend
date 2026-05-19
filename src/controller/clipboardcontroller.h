#ifndef CLIPBOARDCONTROLLER_H
#define CLIPBOARDCONTROLLER_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QVariantList>

/**
 * @brief 剪贴板分享口令嗅探控制器
 *
 * 监听窗口激活事件，在用户切回网盘窗口时检测剪贴板中是否包含分享口令。
 * 如果匹配到口令，发射信号通知 QML 弹出提示卡片。
 * 验证成功后，提供分享文件浏览和保存到网盘的功能。
 */
class ClipboardController : public QObject
{
    Q_OBJECT



    /// 是否正在加载
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    static ClipboardController* instance();

    /// 启用剪贴板监听（登录成功进入主页后调用）
    Q_INVOKABLE void startMonitoring();

    /// 停用剪贴板监听（登出时调用）
    Q_INVOKABLE void stopMonitoring();

    /// 验证分享口令
    Q_INVOKABLE void openShare(const QString &shareKey, const QString &extractCode);

    /// 忽略检测到的口令
    Q_INVOKABLE void dismissShare(const QString &shareKey);

    /// 加载分享文件列表（parentId 为空则加载根目录）
    Q_INVOKABLE void loadShareFiles(int parentId = -1);

    /// 加载用户自己网盘的文件夹列表（用于选择保存位置）
    Q_INVOKABLE void loadMyFolders(int parentId = 0);

    /// 执行保存：将指定分享文件保存到用户网盘的目标文件夹
    Q_INVOKABLE void saveToMyDisk(const QVariantList &sourceFileIds, int targetParentId);

    // Property getters
    bool loading() const { return m_loading; }

signals:
    /// 检测到剪贴板中包含分享口令
    void shareCodeDetected(const QString &shareKey, const QString &extractCode);

    /// 口令验证成功
    void shareVerified(const QString &shareToken,
                       const QString &sharerName,
                       const QString &sharerAvatar,
                       const QString &fileName,
                       int fileId,
                       bool isFolder);

    /// 口令验证失败
    void shareVerifyFailed(const QString &errorMsg);

    /// 检测到是自己的分享
    void ownShareDetected();

    /// 分享文件子节点加载完成（供 QML 树形结构插入）
    void shareChildrenLoaded(int parentId, const QVariantList &children);

    /// 文件夹子节点加载完成（供 QML 树形结构插入子节点）
    void folderChildrenLoaded(int parentId, const QVariantList &children);

    /// 加载状态变化
    void loadingChanged();

    /// 保存成功
    void saveSuccess(const QString &message);

    /// 保存失败
    void saveFailed(const QString &errorMsg);

private:
    explicit ClipboardController(QObject *parent = nullptr);

    void onApplicationStateChanged(Qt::ApplicationState state);
    bool parseShareText(const QString &text, QString &outShareKey, QString &outExtractCode);

    void setLoading(bool loading);

    QString m_lastDetectedKey;
    bool m_enabled = false;
    bool m_loading = false;

    /// 验证成功后存储的 share_token
    QString m_shareToken;

    /// 当前分享的口令
    QString m_currentShareKey;


};

#endif // CLIPBOARDCONTROLLER_H
