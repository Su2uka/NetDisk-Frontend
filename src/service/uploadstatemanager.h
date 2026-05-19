#ifndef UPLOADSTATEMANAGER_H
#define UPLOADSTATEMANAGER_H

#include <QString>
#include <QList>
#include <QPair>
#include <QDateTime>

// ── 分片上传持久化状态 ──
struct MultipartState {
    QString uploadId;
    QString fingerprint;       // "localPath|fileSize|lastModified"
    QString localPath;
    QString targetFolderId;
    QString fileHash;          // SHA-256
    qint64  fileSize = 0;
    qint64  partSize = 8 * 1024 * 1024;  // 8MB
    int     totalParts = 0;
    QList<QPair<int, QString>> completedParts;  // <partNumber, eTag>
    QDateTime createdAt;
};

// ── JSON 文件持久化管理 ──
// 在 %APPDATA%/NetDisk/upload_states/ 目录下为每个任务维护一个 .json 文件
class UploadStateManager
{
public:
    static UploadStateManager* instance();

    /// 生成文件快速指纹: "path|size|mtime"
    static QString fingerprint(const QString &filePath);

    /// 获取状态目录
    static QString stateDir();

    /// 保存分片状态到 JSON
    void saveState(const MultipartState &state);

    /// 按指纹加载状态（用于检测断点续传）
    bool loadByFingerprint(const QString &fingerprint, MultipartState &out);

    /// 按 uploadId 加载状态
    bool loadByUploadId(const QString &uploadId, MultipartState &out);

    /// 删除状态文件
    void removeState(const QString &uploadId);

    /// 加载所有未完成的状态（程序启动时恢复）
    QList<MultipartState> loadAllPending();

    /// 判断 upload_id 是否已过期（默认 7 天）
    static bool isExpired(const MultipartState &state, int maxDays = 7);

private:
    UploadStateManager() = default;
    QString stateFilePath(const QString &uploadId) const;
};

#endif // UPLOADSTATEMANAGER_H
