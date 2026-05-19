#include "downloadcontroller.h"
#include "../network/networkmanager.h"

#include <QUuid>
#include <QDir>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QDebug>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

// ── 单例 ──
DownloadController* DownloadController::instance()
{
    static DownloadController inst;
    return &inst;
}

DownloadController::DownloadController(QObject *parent)
    : QObject(parent)
    , m_model(new DownloadListModel(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
}

DownloadListModel* DownloadController::model() const
{
    return m_model;
}

int DownloadController::activeCount() const
{
    return m_activeCount;
}

int DownloadController::pendingCount() const
{
    int count = 0;
    for (const auto &t : m_model->tasks()) {
        if (t.status == DownloadStatus::Queued)
            count++;
    }
    return count;
}

int DownloadController::maxConcurrent() const
{
    return m_maxConcurrent;
}

void DownloadController::setMaxConcurrent(int n)
{
    m_maxConcurrent = qMax(1, n);
}

// ═══════════════════════════════════════════════════════════
//  入队接口
// ═══════════════════════════════════════════════════════════

void DownloadController::enqueueFile(const QString &fileId,
                                      const QString &fileName,
                                      const QString &preSignedUrl,
                                      const QString &localSavePath,
                                      qint64 totalBytes)
{
    DownloadTask task;
    task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.fileId = fileId;
    task.fileName = fileName;
    task.preSignedUrl = preSignedUrl;
    task.localSavePath = localSavePath;
    task.totalBytes = totalBytes;

    m_model->appendTask(task);
    emit pendingCountChanged();

    saveDownloadStates();
    scheduleNext();
}

void DownloadController::enqueueFolder(const QVariantList &flatFileList,
                                        const QString &localRootPath)
{
    if (flatFileList.isEmpty())
        return;

    QSet<QString> dirsToCreate;
    QList<DownloadTask> tasks;

    for (const QVariant &item : flatFileList) {
        QVariantMap map = item.toMap();
        QString fileId       = map["fileId"].toString();
        QString relativePath = map["relativePath"].toString();   // 如 "A/B"
        QString fileName     = map["fileName"].toString();
        QString preSignedUrl = map["preSignedUrl"].toString();
        qint64  fileSize     = map["fileSize"].toLongLong();

        // 收集需要创建的目录
        if (!relativePath.isEmpty()) {
            dirsToCreate.insert(relativePath);
        }

        // 构造完整本地路径
        QString fullDir = localRootPath;
        if (!relativePath.isEmpty()) {
            fullDir += "/" + relativePath;
        }
        QString localSavePath = fullDir + "/" + fileName;

        DownloadTask task;
        task.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.fileId = fileId;
        task.fileName = fileName;
        task.preSignedUrl = preSignedUrl;
        task.localSavePath = localSavePath;
        task.totalBytes = fileSize;

        tasks.append(task);
    }

    // 先在本地重建目录结构
    QDir rootDir(localRootPath);
    for (const QString &dirPath : dirsToCreate) {
        rootDir.mkpath(dirPath);
    }

    // 批量入队
    m_model->appendTasks(tasks);
    emit pendingCountChanged();

    saveDownloadStates();
    scheduleNext();
}

// ═══════════════════════════════════════════════════════════
//  调度与下载核心
// ═══════════════════════════════════════════════════════════

void DownloadController::scheduleNext()
{
    auto &tasks = m_model->tasks();

    for (int i = 0; i < tasks.count() && m_activeCount < m_maxConcurrent; ++i) {
        if (tasks[i].status == DownloadStatus::Queued) {
            doDownload(tasks[i].taskId);
        }
    }
}

void DownloadController::doDownload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    DownloadTask &task = m_model->tasks()[row];

    // ── 1. 确保目标目录存在 ──
    QFileInfo fi(task.localSavePath);
    QDir().mkpath(fi.absolutePath());

    // ── 2. 打开本地文件 ──
    QFile *file = new QFile(task.localSavePath);

    // 判断是否是断点续传：使用磁盘上的实际文件大小作为可靠偏移量
    qint64 fileOffset = 0;
    if (task.receivedBytes > 0 && file->exists()) {
        fileOffset = file->size();  // 磁盘上实际已写入的字节数
    }

    QIODevice::OpenMode mode = (fileOffset > 0)
                                   ? QIODevice::WriteOnly | QIODevice::Append
                                   : QIODevice::WriteOnly;

    if (!file->open(mode)) {
        delete file;
        m_model->updateError(taskId, "无法创建本地文件: " + task.localSavePath);
        emit taskFailed(taskId, task.fileName, "无法创建本地文件");
        return;
    }
    task.file = file;

    // 同步 receivedBytes 为磁盘上实际的文件大小
    task.receivedBytes = fileOffset;

    // ── 3. 发起 GET 请求（预签名直链，无需 Token） ──
    QNetworkRequest request(QUrl(task.preSignedUrl));

    // 如果有已下载字节，使用 Range 头断点续传
    if (fileOffset > 0) {
        QString rangeHeader = QString("bytes=%1-").arg(fileOffset);
        request.setRawHeader("Range", rangeHeader.toLatin1());
        qDebug() << "[下载] 断点续传:" << taskId
                 << "from byte" << fileOffset;
    }

    QNetworkReply *reply = m_networkManager->get(request);
    task.reply = reply;

    // 更新状态
    m_model->updateStatus(taskId, DownloadStatus::Downloading);
    m_activeCount++;
    emit activeCountChanged();
    emit pendingCountChanged();

    // 记住续传偏移量（用于计算总进度）
    // 使用 shared_ptr 以便在 metaDataChanged 中可以修改
    auto resumeOffset = std::make_shared<qint64>(fileOffset);

    // ── 3.5. 检查服务端是否支持 Range ──
    // 如果我们请求了 Range 但服务端返回 200（非 206），说明不支持断点续传
    // 需要清空已有文件，从头写入
    if (fileOffset > 0) {
        connect(reply, &QNetworkReply::metaDataChanged, this, [this, taskId, resumeOffset]() {
            int row = m_model->findByTaskId(taskId);
            if (row < 0) return;

            DownloadTask &task = m_model->tasks()[row];
            if (!task.reply) return;

            int statusCode = task.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 200) {
                // 服务端不支持 Range，返回了完整文件
                qDebug() << "[下载] 服务端不支持 Range，从头下载:" << taskId;
                if (task.file) {
                    task.file->resize(0);
                    task.file->seek(0);
                }
                task.receivedBytes = 0;
                *resumeOffset = 0;  // 重置偏移量
            }
        });
    }

    // ── 4. readyRead — 边下边写（内存安全核心）──
    connect(reply, &QNetworkReply::readyRead, this, [this, taskId]() {
        int row = m_model->findByTaskId(taskId);
        if (row < 0) return;

        DownloadTask &task = m_model->tasks()[row];
        if (task.file && task.reply) {
            QByteArray chunk = task.reply->readAll();
            task.file->write(chunk);
        }
    });

    // ── 5. downloadProgress — 精准刷新进度 ──
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, taskId, resumeOffset](qint64 received, qint64 total) {
        qint64 offset = *resumeOffset;
        qint64 actualReceived = offset + received;
        qint64 actualTotal = (total > 0) ? offset + total : 0;
        int percent = (actualTotal > 0)
                          ? static_cast<int>(actualReceived * 100 / actualTotal)
                          : 0;
        m_model->updateProgress(taskId, actualReceived, actualTotal, percent);
        emit taskProgress(taskId, percent);
    });

    // ── 6. finished — 收尾处理 ──
    connect(reply, &QNetworkReply::finished, this, [this, taskId, resumeOffset]() {
        int row = m_model->findByTaskId(taskId);
        if (row < 0) return;

        DownloadTask &task = m_model->tasks()[row];

        // 如果是暂停导致的 abort，不在这里处理
        if (task.status == DownloadStatus::Paused) {
            return;
        }

        bool hasError = (task.reply && task.reply->error() != QNetworkReply::NoError);

        // 检查是否是 OperationCanceledError（用户主动取消/暂停导致的）
        if (task.reply && task.reply->error() == QNetworkReply::OperationCanceledError) {
            // 暂停或取消导致的中断，不算错误
            return;
        }

        // 读取网络缓冲区中可能残留的数据
        if (!hasError && task.file && task.reply) {
            QByteArray remaining = task.reply->readAll();
            if (!remaining.isEmpty()) {
                task.file->write(remaining);
            }
        }

        // 关闭文件流
        if (task.file) {
            task.file->close();
            delete task.file;
            task.file = nullptr;
        }

        // 清理网络句柄
        if (task.reply) {
            task.reply->deleteLater();
        }

        if (hasError) {
            QString errMsg = task.reply ? task.reply->errorString() : "未知网络错误";
            m_model->updateError(taskId, errMsg);
            emit taskFailed(taskId, task.fileName, errMsg);

            // 下载失败，删除不完整的文件
            QFile::remove(task.localSavePath);
        } else {
            qint64 finalTotal = task.totalBytes > 0 ? task.totalBytes : task.receivedBytes;
            m_model->updateStatus(taskId, DownloadStatus::Success);
            m_model->updateProgress(taskId, finalTotal, finalTotal, 100);
            emit taskSuccess(taskId, task.fileName);
        }

        saveDownloadStates();  // 成功后更新状态文件
        releaseSlot();
        scheduleNext();
    });
}

// ═══════════════════════════════════════════════════════════
//  暂停 / 恢复 / 取消
// ═══════════════════════════════════════════════════════════

void DownloadController::startDownload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    DownloadTask &task = m_model->tasks()[row];

    if (task.status != DownloadStatus::Paused)
        return;

    // 如果有 fileId，重新向后端获取预签名 URL（旧 URL 可能已过期）
    if (!task.fileId.isEmpty()) {
        m_model->updateStatus(taskId, DownloadStatus::Queued);
        emit pendingCountChanged();

        QString apiPath = "/files/" + task.fileId + "/download_url";
        NetworkManager::instance()->get(
            apiPath, QJsonObject(),
            [this, taskId](const QJsonObject &data) {
                int row = m_model->findByTaskId(taskId);
                if (row < 0) return;

                DownloadTask &task = m_model->tasks()[row];
                if (task.status != DownloadStatus::Queued) return;

                QString newUrl = data["download_url"].toString();
                if (!newUrl.isEmpty()) {
                    task.preSignedUrl = newUrl;
                    qDebug() << "[下载] 已获取新的预签名 URL:" << task.fileName;
                }
                scheduleNext();
            },
            [this, taskId](const QString &errMsg) {
                qWarning() << "[下载] 获取新预签名 URL 失败:" << errMsg;
                // 尝试用旧 URL 继续
                scheduleNext();
            }
        );
    } else {
        // 没有 fileId，直接用旧 URL 尝试
        task.status = DownloadStatus::Queued;
        m_model->updateStatus(taskId, DownloadStatus::Queued);
        emit pendingCountChanged();
        scheduleNext();
    }
}

void DownloadController::pauseDownload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    DownloadTask &task = m_model->tasks()[row];

    if (task.status != DownloadStatus::Downloading)
        return;

    // 更新状态为暂停（在 abort 之前，防止 finished 回调误判）
    m_model->updateStatus(taskId, DownloadStatus::Paused);

    // 关闭文件流
    if (task.file) {
        task.file->flush();
        task.file->close();
        delete task.file;
        task.file = nullptr;
    }

    // 掐断网络连接
    if (task.reply) {
        task.reply->abort();
        task.reply->deleteLater();
        task.reply = nullptr;
    }

    releaseSlot();
    saveDownloadStates();
    scheduleNext();
}

void DownloadController::cancelDownload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    DownloadTask &task = m_model->tasks()[row];
    bool wasActive = (task.status == DownloadStatus::Downloading);

    // 清理资源
    cleanupTaskResources(task);

    // 删除已下载的临时文件
    if (QFile::exists(task.localSavePath)) {
        QFile::remove(task.localSavePath);
    }

    // 从模型中移除
    m_model->removeTask(taskId);
    saveDownloadStates();  // 取消后更新状态文件

    if (wasActive) {
        releaseSlot();
    }
    emit pendingCountChanged();

    scheduleNext();
}

void DownloadController::clearFinished()
{
    m_model->removeFinished();
    saveDownloadStates();
}

void DownloadController::cancelAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == DownloadStatus::Queued ||
            task.status == DownloadStatus::Downloading ||
            task.status == DownloadStatus::Paused) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        cancelDownload(id);
    }
}

void DownloadController::resumeAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == DownloadStatus::Paused) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        startDownload(id);
    }
}

void DownloadController::pauseAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == DownloadStatus::Downloading) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        pauseDownload(id);
    }
}

// ═══════════════════════════════════════════════════════════
//  内部辅助
// ═══════════════════════════════════════════════════════════

void DownloadController::releaseSlot()
{
    m_activeCount = qMax(0, m_activeCount - 1);
    emit activeCountChanged();
}

void DownloadController::cleanupTaskResources(DownloadTask &task)
{
    if (task.file) {
        task.file->close();
        delete task.file;
        task.file = nullptr;
    }
    if (task.reply) {
        task.reply->abort();
        task.reply->deleteLater();
        task.reply = nullptr;
    }
}

// ═══════════════════════════════════════════════════════════
//  下载状态持久化
// ═══════════════════════════════════════════════════════════

QString DownloadController::downloadStatesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/download_states.json";
}

void DownloadController::saveDownloadStates()
{
    QJsonArray arr;
    for (const auto &task : m_model->tasks()) {
        // 只保存未完成的任务（排队、下载中、暂停）
        if (task.status == DownloadStatus::Queued ||
            task.status == DownloadStatus::Downloading ||
            task.status == DownloadStatus::Paused) {
            if (task.fileId.isEmpty()) continue;  // 没有 fileId 无法恢复

            QJsonObject obj;
            obj["file_id"]        = task.fileId;
            obj["file_name"]      = task.fileName;
            obj["local_save_path"] = task.localSavePath;
            obj["total_bytes"]    = task.totalBytes;
            obj["received_bytes"] = task.receivedBytes;
            arr.append(obj);
        }
    }

    QFile file(downloadStatesFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        file.close();
    }
}

void DownloadController::restorePendingDownloads()
{
    QFile file(downloadStatesFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return;

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    file.close();

    if (arr.isEmpty()) {
        qDebug() << "[恢复] 没有未完成的下载任务";
        return;
    }

    qDebug() << "[恢复] 发现" << arr.size() << "个未完成的下载任务";

    for (const auto &v : arr) {
        QJsonObject obj = v.toObject();
        QString fileId       = obj["file_id"].toString();
        QString fileName     = obj["file_name"].toString();
        QString localSavePath = obj["local_save_path"].toString();
        qint64  totalBytes   = obj["total_bytes"].toVariant().toLongLong();
        qint64  receivedBytes = obj["received_bytes"].toVariant().toLongLong();

        if (fileId.isEmpty()) continue;

        // 检查本地文件是否存在（确定已下载字节数）
        QFileInfo fi(localSavePath);
        qint64 actualBytes = fi.exists() ? fi.size() : 0;

        DownloadTask task;
        task.taskId       = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.fileId       = fileId;
        task.fileName     = fileName;
        task.localSavePath = localSavePath;
        task.totalBytes   = totalBytes;
        task.receivedBytes = actualBytes;  // 用磁盘实际大小
        task.progress     = (totalBytes > 0) ? static_cast<int>(actualBytes * 100 / totalBytes) : 0;
        task.status       = DownloadStatus::Paused;  // 以暂停状态恢复

        m_model->appendTask(task);

        qDebug() << "[恢复] 下载任务:" << fileName
                 << "已下载:" << actualBytes << "/" << totalBytes;
    }

    // 清除状态文件（已加载到内存）
    QFile::remove(downloadStatesFilePath());

    emit pendingCountChanged();
}
