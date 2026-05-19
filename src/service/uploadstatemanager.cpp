#include "uploadstatemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

// ── 单例 ──
UploadStateManager* UploadStateManager::instance()
{
    static UploadStateManager inst;
    return &inst;
}

// ── 状态目录 ──
QString UploadStateManager::stateDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/upload_states";
    QDir().mkpath(dir);
    return dir;
}

// ── 快速指纹 ──
QString UploadStateManager::fingerprint(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) return {};
    // "绝对路径|文件大小|最后修改时间"
    return fi.absoluteFilePath() + "|"
           + QString::number(fi.size()) + "|"
           + fi.lastModified().toString(Qt::ISODate);
}

// ── 状态文件路径 ──
QString UploadStateManager::stateFilePath(const QString &uploadId) const
{
    return stateDir() + "/" + uploadId + ".upload.json";
}

// ── 保存状态 ──
void UploadStateManager::saveState(const MultipartState &state)
{
    QJsonObject obj;
    obj["upload_id"]       = state.uploadId;
    obj["fingerprint"]     = state.fingerprint;
    obj["local_path"]      = state.localPath;
    obj["target_folder_id"]= state.targetFolderId;
    obj["file_hash"]       = state.fileHash;
    obj["file_size"]       = state.fileSize;
    obj["part_size"]       = state.partSize;
    obj["total_parts"]     = state.totalParts;
    obj["created_at"]      = state.createdAt.toString(Qt::ISODate);

    QJsonArray partsArr;
    for (const auto &p : state.completedParts) {
        QJsonObject partObj;
        partObj["part_number"] = p.first;
        partObj["etag"]        = p.second;
        partsArr.append(partObj);
    }
    obj["completed_parts"] = partsArr;

    QFile file(stateFilePath(state.uploadId));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.close();
    } else {
        qWarning() << "[UploadState] 无法写入状态文件:" << file.fileName();
    }
}

// ── 从 JSON 解析 MultipartState ──
static bool parseStateFile(const QString &filePath, MultipartState &out)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    if (obj.isEmpty() || !obj.contains("upload_id"))
        return false;

    out.uploadId       = obj["upload_id"].toString();
    out.fingerprint    = obj["fingerprint"].toString();
    out.localPath      = obj["local_path"].toString();
    out.targetFolderId = obj["target_folder_id"].toString();
    out.fileHash       = obj["file_hash"].toString();
    out.fileSize       = obj["file_size"].toVariant().toLongLong();
    out.partSize       = obj["part_size"].toVariant().toLongLong();
    out.totalParts     = obj["total_parts"].toInt();
    out.createdAt      = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);

    out.completedParts.clear();
    QJsonArray partsArr = obj["completed_parts"].toArray();
    for (const auto &v : partsArr) {
        QJsonObject p = v.toObject();
        out.completedParts.append({p["part_number"].toInt(), p["etag"].toString()});
    }

    return true;
}

// ── 按指纹加载 ──
bool UploadStateManager::loadByFingerprint(const QString &fp, MultipartState &out)
{
    QDir dir(stateDir());
    QStringList files = dir.entryList({"*.upload.json"}, QDir::Files);

    for (const QString &f : files) {
        MultipartState state;
        if (parseStateFile(dir.absoluteFilePath(f), state)) {
            if (state.fingerprint == fp) {
                out = state;
                return true;
            }
        }
    }
    return false;
}

// ── 按 uploadId 加载 ──
bool UploadStateManager::loadByUploadId(const QString &uploadId, MultipartState &out)
{
    return parseStateFile(stateFilePath(uploadId), out);
}

// ── 删除状态 ──
void UploadStateManager::removeState(const QString &uploadId)
{
    QString path = stateFilePath(uploadId);
    if (QFile::exists(path)) {
        QFile::remove(path);
        qDebug() << "[UploadState] 已清理状态文件:" << uploadId;
    }
}

// ── 加载所有未完成 ──
QList<MultipartState> UploadStateManager::loadAllPending()
{
    QList<MultipartState> result;
    QDir dir(stateDir());
    QStringList files = dir.entryList({"*.upload.json"}, QDir::Files);

    for (const QString &f : files) {
        MultipartState state;
        if (parseStateFile(dir.absoluteFilePath(f), state)) {
            if (!isExpired(state)) {
                result.append(state);
            } else {
                // 过期的自动清理
                QFile::remove(dir.absoluteFilePath(f));
                qDebug() << "[UploadState] 清理过期状态:" << state.uploadId;
            }
        }
    }
    return result;
}

// ── 判断是否过期 ──
bool UploadStateManager::isExpired(const MultipartState &state, int maxDays)
{
    if (!state.createdAt.isValid()) return true;
    return state.createdAt.daysTo(QDateTime::currentDateTime()) > maxDays;
}
