#ifndef UPLOADLISTMODEL_H
#define UPLOADLISTMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QNetworkReply>
#include <QMap>

// ── 单个上传任务的状态枚举 ──
enum class UploadStatus {
    Queued,     // 排队等待中
    Hashing,    // 正在子线程计算 SHA-256
    Uploading,  // 正在网络上传
    Paused,     // 用户暂停
    Success,    // 上传成功
    Failed,     // 上传失败
    Canceled    // 用户手动取消
};

// ── 单个上传任务的数据结构 ──
struct UploadTask {
    QString taskId;                    // 唯一任务 ID（UUID）
    QString localPath;                 // 本地文件绝对路径
    QString fileName;                  // 文件名
    qint64  fileSize = 0;              // 文件大小（字节）
    QString targetFolderId;            // 服务器端的目标目录 ID
    QString sha256;                    // 文件 SHA-256 哈希（秒传校验用）
    UploadStatus status = UploadStatus::Queued;
    int     progress = 0;              // 上传进度百分比 0~100
    qint64  receivedBytes = 0;         // 已上传字节数
    QString errorMsg;                  // 错误信息（仅 Failed / Canceled 时有值）
    QPointer<QNetworkReply> reply;     // 网络请求句柄（单文件上传 / 取消用）

    // ── 分片上传字段 ──
    bool    isMultipart = false;       // 是否使用分片上传
    QString uploadId;                  // MinIO 分片上传会话 ID
    qint64  partSize = 8 * 1024 * 1024;  // 每片 8MB
    int     totalParts = 0;
    int     completedPartsCount = 0;
    int     activePartsCount = 0;      // 当前正在上传的分片数
    QList<QPair<int, QString>> eTags;  // 已完成的 <partNumber, eTag>
    QMap<int, QString> presignedUrls;  // partNumber -> 预签名 URL
    QList<QPointer<QNetworkReply>> activePartReplies;  // 当前活跃的分片请求句柄
};

// ── 上传列表数据模型 ──
// 继承 QAbstractListModel，实现精准的局部刷新
class UploadListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    // QML 可通过这些角色名访问每一行的数据
    enum UploadRoles {
        TaskIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileSizeRole,
        ReceivedBytesRole,
        StatusRole,
        ProgressRole,
        ErrorMsgRole
    };
    Q_ENUM(UploadRoles)

    explicit UploadListModel(QObject *parent = nullptr);

    // ── QAbstractListModel 必须实现的三个虚函数 ──
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── 供 UploadController 调用的精准更新接口 ──

    /// 在末尾追加一批任务
    void appendTasks(const QList<UploadTask> &tasks);

    /// 更新某个任务的状态（精准刷新 StatusRole）
    void updateStatus(const QString &taskId, UploadStatus status);

    /// 更新某个任务的进度（精准刷新 ProgressRole）
    void updateProgress(const QString &taskId, int percent);

    /// 更新某个任务的错误信息（精准刷新 ErrorMsgRole + StatusRole）
    void updateError(const QString &taskId, const QString &errorMsg);

    /// 移除某个任务（按 taskId）
    void removeTask(const QString &taskId);

    /// 移除所有已完成 / 已失败 / 已取消的任务
    void removeFinished();

    // ── 直接访问底层数据（供 UploadController 内部调度使用）──

    QList<UploadTask>& tasks();
    const QList<UploadTask>& tasks() const;

    /// 按 taskId 查找任务在列表中的索引，找不到返回 -1
    int findByTaskId(const QString &taskId) const;

signals:
    void countChanged();

private:
    QList<UploadTask> m_tasks;
};

#endif // UPLOADLISTMODEL_H
