#ifndef DOWNLOADLISTMODEL_H
#define DOWNLOADLISTMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QNetworkReply>
#include <QFile>

// ── 单个下载任务的状态枚举 ──
enum class DownloadStatus {
    Queued,       // 排队等待
    Downloading,  // 下载中
    Paused,       // 已暂停
    Success,      // 下载成功
    Failed        // 下载失败
};

// ── 单个下载任务的数据结构 ──
struct DownloadTask {
    QString taskId;                    // 唯一任务 ID（UUID）
    QString fileId;                    // 服务端文件 ID（用于重启后重新获取预签名 URL）
    QString parentId;                  // 所在文件夹 ID（用于最近文件跳转）
    QString fileName;                  // 文件名
    QString localSavePath;             // 本地保存完整路径
    QString preSignedUrl;              // MinIO/S3 预签名下载直链
    qint64  totalBytes = 0;            // 文件总大小（字节）
    qint64  receivedBytes = 0;         // 已接收字节
    int     progress = 0;              // 下载进度百分比 0~100
    DownloadStatus status = DownloadStatus::Queued;
    QString errorMsg;                  // 错误信息
    bool parentIdKnown = true;         // 是否可以安全跳转到远端所在目录
    QPointer<QNetworkReply> reply;     // 网络请求句柄（用于 abort）
    QFile*  file = nullptr;            // 本地文件写入句柄（边下边写）
};

// ── 下载列表数据模型 ──
// 继承 QAbstractListModel，实现精准的局部刷新
class DownloadListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    // QML 可通过这些角色名访问每一行的数据
    enum DownloadRoles : int {
        TaskIdRole = Qt::UserRole + 1,
        FileNameRole,
        TotalBytesRole,
        ReceivedBytesRole,
        ProgressRole,
        StatusRole,
        ErrorMsgRole,
    };
    Q_ENUM(DownloadRoles)

    explicit DownloadListModel(QObject *parent = nullptr);

    // ── QAbstractListModel 必须实现的三个虚函数 ──
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── 供 DownloadController 调用的精准更新接口 ──

    /// 追加单个任务
    void appendTask(const DownloadTask &task);

    /// 追加一批任务
    void appendTasks(const QList<DownloadTask> &tasks);

    /// 更新进度（精准刷新 ReceivedBytesRole + ProgressRole）
    void updateProgress(const QString &taskId, qint64 receivedBytes, qint64 totalBytes, int percent);

    /// 更新状态（精准刷新 StatusRole）
    void updateStatus(const QString &taskId, DownloadStatus status);

    /// 更新错误信息（精准刷新 ErrorMsgRole + StatusRole）
    void updateError(const QString &taskId, const QString &errorMsg);

    /// 移除某个任务
    void removeTask(const QString &taskId);

    /// 移除所有已完成 / 已失败的任务
    void removeFinished();

    // ── 直接访问底层数据（供 DownloadController 内部调度使用）──

    QList<DownloadTask>& tasks();
    const QList<DownloadTask>& tasks() const;

    /// 按 taskId 查找任务在列表中的索引，找不到返回 -1
    int findByTaskId(const QString &taskId) const;

signals:
    void countChanged();

private:
    QList<DownloadTask> m_tasks;
};

#endif // DOWNLOADLISTMODEL_H
