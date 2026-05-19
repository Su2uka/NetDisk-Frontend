#ifndef DOWNLOADCONTROLLER_H
#define DOWNLOADCONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include "../model/downloadlistmodel.h"

class DownloadController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(DownloadListModel* model READ model CONSTANT)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY activeCountChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)

public:
    static DownloadController* instance();

    DownloadListModel* model() const;
    int activeCount() const;
    int pendingCount() const;

    // ── 核心接口 ──

    /// 入队单文件下载任务
    Q_INVOKABLE void enqueueFile(const QString &fileId,
                                  const QString &fileName,
                                  const QString &preSignedUrl,
                                  const QString &localSavePath,
                                  qint64 totalBytes);

    /// 入队文件夹下载（接收扁平化文件树列表）
    /// 每项 QVariantMap 需包含: relativePath, fileName, preSignedUrl, fileSize
    Q_INVOKABLE void enqueueFolder(const QVariantList &flatFileList,
                                    const QString &localRootPath);

    /// 恢复暂停的任务（从头开始下载）
    Q_INVOKABLE void startDownload(const QString &taskId);

    /// 暂停下载（abort 网络连接）
    Q_INVOKABLE void pauseDownload(const QString &taskId);

    /// 取消下载（abort + 删除临时文件 + 移除任务）
    Q_INVOKABLE void cancelDownload(const QString &taskId);

    /// 批量操作
    Q_INVOKABLE void cancelAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void pauseAll();

    /// 清理已完成 / 已失败的记录
    Q_INVOKABLE void clearFinished();

    /// 程序启动/登录后调用：从 JSON 恢复未完成的下载任务
    Q_INVOKABLE void restorePendingDownloads();

    int maxConcurrent() const;
    void setMaxConcurrent(int n);

signals:
    void activeCountChanged();
    void pendingCountChanged();

    void taskProgress(const QString &taskId, int percent);
    void taskSuccess(const QString &taskId, const QString &fileName);
    void taskFailed(const QString &taskId, const QString &fileName, const QString &errorMsg);

private:
    explicit DownloadController(QObject *parent = nullptr);

    /// 调度队列：启动排队中的任务直到达到并发上限
    void scheduleNext();

    /// 执行实际下载（创建网络请求、绑定 readyRead/downloadProgress/finished）
    void doDownload(const QString &taskId);

    /// 释放一个并发槽位
    void releaseSlot();

    /// 清理任务的网络和文件资源
    void cleanupTaskResources(DownloadTask &task);

    /// 持久化/恢复下载状态
    void saveDownloadStates();
    QString downloadStatesFilePath() const;

    DownloadListModel* m_model = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    int m_maxConcurrent = 3;
    int m_activeCount = 0;
};

#endif // DOWNLOADCONTROLLER_H
