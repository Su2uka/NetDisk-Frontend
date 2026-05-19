#ifndef UPLOADCONTROLLER_H
#define UPLOADCONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QNetworkAccessManager>
#include "../model/uploadlistmodel.h"

class UploadStateManager;

// ── 子线程扫描文件夹的返回结构 ──
struct FolderScanResult {
    // 按广度优先排序的子目录列表
    // QPair<相对路径, 目录名>
    // 例如: {"src", "src"},  {"src/core", "core"}
    QList<QPair<QString, QString>> dirs;

    // 所有文件列表
    // QPair<本地绝对路径, 所属目录的相对路径>
    // 例如: {"C:/.../main.cpp", "src"},  {"C:/.../README.md", ""}
    QList<QPair<QString, QString>> files;
};

class UploadController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(UploadListModel* model READ model CONSTANT)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY activeCountChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)

public:
    static UploadController* instance();

    UploadListModel* model() const;
    int activeCount() const;
    int pendingCount() const;

    // ── 核心接口 ──

    /// 将一批文件加入上传队列（由 FileController 调用）
    Q_INVOKABLE void enqueueFiles(const QList<QUrl> &fileUrls, const QString &targetFolderId);

    /// 将一个文件夹整体上传（先创建远程目录结构，再入队文件）
    Q_INVOKABLE void enqueueFolder(const QUrl &folderUrl, const QString &targetFolderId);

    /// 取消某个任务
    Q_INVOKABLE void cancelTask(const QString &taskId);

    /// 暂停某个上传任务
    Q_INVOKABLE void pauseTask(const QString &taskId);

    /// 恢复某个上传任务
    Q_INVOKABLE void resumeTask(const QString &taskId);

    /// 取消所有上传任务
    Q_INVOKABLE void cancelAll();

    /// 批量暂停/恢复
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();

    /// 程序启动/登录后调用：从 JSON 恢复未完成的分片上传任务
    Q_INVOKABLE void restorePendingUploads();

    /// 清理已完成 / 已失败 / 已取消的记录
    Q_INVOKABLE void clearFinished();

    int maxConcurrent() const;
    void setMaxConcurrent(int n);

signals:
    void activeCountChanged();
    void pendingCountChanged();

    void taskProgress(const QString &taskId, int percent);
    void taskSuccess(const QString &taskId, const QString &fileName);
    void taskFailed(const QString &taskId, const QString &fileName, const QString &errorMsg);

private:
    explicit UploadController(QObject *parent = nullptr);

    void scheduleNext();
    void startHashing(const QString &taskId);
    void startUpload(const QString &taskId);
    void releaseSlot();

    // ── 分片上传核心 ──
    void initMultipartUpload(const QString &taskId);
    void uploadNextParts(const QString &taskId);
    void uploadSinglePart(const QString &taskId, int partNumber);
    void onPartCompleted(const QString &taskId, int partNumber, const QString &etag);
    void completeMultipartUpload(const QString &taskId);
    void abortMultipartUpload(const QString &uploadId);

    /// 扫描完成后，链式创建远程目录，最终把文件入队
    void processScannedFolder(const FolderScanResult &result,
                              const QString &rootFolderName,
                              const QString &parentFolderId);

    /// 递归创建子目录的链式异步方法
    void createDirsRecursive(const FolderScanResult &result,
                             int dirIndex,
                             QMap<QString, QString> *pathToIdMap,
                             const QString &parentFolderId);

    UploadListModel* m_model = nullptr;
    UploadStateManager* m_stateManager = nullptr;
    QNetworkAccessManager* m_partNam = nullptr;  // 分片上传专用
    int m_maxConcurrent = 3;
    int m_activeCount = 0;
    int m_maxPartsPerTask = 3;  // 每个任务的分片并发数
};

#endif // UPLOADCONTROLLER_H
