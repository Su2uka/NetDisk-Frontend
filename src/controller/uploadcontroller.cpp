#include "uploadcontroller.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QUuid>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include "../network/networkmanager.h"
#include "../service/uploadstatemanager.h"
#include "settingscontroller.h"

// ════════════════════════════════════════════
//  单例
// ════════════════════════════════════════════

UploadController::UploadController(QObject *parent)
    : QObject{parent}
{
    m_model = new UploadListModel(this);
    m_stateManager = UploadStateManager::instance();
    m_partNam = new QNetworkAccessManager(this);
}

UploadController* UploadController::instance()
{
    static UploadController inst;
    return &inst;
}

// ════════════════════════════════════════════
//  属性
// ════════════════════════════════════════════

UploadListModel* UploadController::model() const
{
    return m_model;
}

int UploadController::activeCount() const
{
    return m_activeCount;
}

int UploadController::pendingCount() const
{
    int count = 0;
    for (const auto &task : m_model->tasks()) {
        if (task.status == UploadStatus::Queued)
            count++;
    }
    return count;
}

int UploadController::maxConcurrent() const
{
    return m_maxConcurrent;
}

void UploadController::setMaxConcurrent(int n)
{
    m_maxConcurrent = qMax(1, n);
}

// ════════════════════════════════════════════
//  核心接口：入队
// ════════════════════════════════════════════

void UploadController::enqueueFiles(const QList<QUrl> &fileUrls, const QString &targetFolderId)
{
    QList<UploadTask> newTasks;

    for (const QUrl &url : fileUrls) {
        QString localPath = url.toLocalFile();
        QFileInfo fi(localPath);

        if (!fi.exists() || !fi.isFile()) {
            qWarning() << "[队列] 跳过不存在的文件:" << localPath;
            continue;
        }

        UploadTask task;
        task.taskId         = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.localPath      = fi.absoluteFilePath();
        task.fileName       = fi.fileName();
        task.fileSize       = fi.size();
        task.targetFolderId = targetFolderId;
        task.status         = UploadStatus::Queued;
        task.progress       = 0;

        newTasks.append(task);
        qDebug() << "[队列] 入队:" << task.fileName
                 << "大小:" << task.fileSize
                 << "目标目录:" << task.targetFolderId;
    }

    // 通过 Model 的 appendTasks 批量插入，触发 beginInsertRows / endInsertRows
    m_model->appendTasks(newTasks);

    emit pendingCountChanged();

    // 如果关闭了自动开始上传，将新任务标记为暂停状态
    if (!SettingsController::instance()->autoStartUpload()) {
        for (const auto &t : newTasks) {
            m_model->updateStatus(t.taskId, UploadStatus::Paused);
        }
        return;
    }

    // 入队完成后，尝试调度
    scheduleNext();
}

void UploadController::enqueueFolder(const QUrl &folderUrl, const QString &targetFolderId)
{
    QString localPath = folderUrl.toLocalFile();
    QDir dir(localPath);

    if (!dir.exists()) {
        qWarning() << "[文件夹上传] 文件夹不存在:" << localPath;
        return;
    }

    QString rootFolderName = dir.dirName();
    qDebug() << "[文件夹上传] 开始在子线程中扫描:" << rootFolderName;

    // ── 子线程：同时收集目录和文件，记录相对路径 ──
    QFuture<FolderScanResult> future = QtConcurrent::run([localPath]() -> FolderScanResult {
        FolderScanResult result;
        QDir rootDir(localPath);

        // 1. 收集所有子目录（广度优先天然排序：父目录一定排在子目录前面）
        QDirIterator dirIt(localPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (dirIt.hasNext()) {
            dirIt.next();
            QString relativePath = rootDir.relativeFilePath(dirIt.filePath());
            QString dirName = dirIt.fileName();
            result.dirs.append({relativePath, dirName});
        }

        // 按路径深度排序，确保父目录在前（"src" 在 "src/core" 前面）
        std::sort(result.dirs.begin(), result.dirs.end(),
                  [](const QPair<QString,QString> &a, const QPair<QString,QString> &b) {
                      return a.first.count('/') < b.first.count('/');
                  });

        // 2. 收集所有文件
        QDirIterator fileIt(localPath, QDir::Files, QDirIterator::Subdirectories);
        while (fileIt.hasNext()) {
            fileIt.next();
            QString absolutePath = fileIt.filePath();
            // 文件所属目录的相对路径（相对于根文件夹）
            QString fileRelDir = rootDir.relativeFilePath(QFileInfo(absolutePath).path());
            if (fileRelDir == ".")
                fileRelDir = ""; // 根目录下的文件
            result.files.append({absolutePath, fileRelDir});
        }

        return result;
    });

    // ── 主线程回调：开始链式创建目录 ──
    auto *watcher = new QFutureWatcher<FolderScanResult>(this);
    connect(watcher, &QFutureWatcher<FolderScanResult>::finished, this,
            [this, watcher, rootFolderName, targetFolderId]() {
                FolderScanResult result = watcher->result();
                watcher->deleteLater();

                qDebug() << "[文件夹上传] 扫描完成：" << result.dirs.size() << "个子目录，" << result.files.size() << "个文件";

                // 启动异步目录创建流程
                processScannedFolder(result, rootFolderName, targetFolderId);
            });

    watcher->setFuture(future);
}

// ════════════════════════════════════════════
//  文件夹上传：链式创建远程目录
// ════════════════════════════════════════════

void UploadController::processScannedFolder(const FolderScanResult &result,
                                            const QString &rootFolderName,
                                            const QString &parentFolderId)
{
    // pathToIdMap 记录 "相对路径" → "服务端 folder ID" 的映射
    // 使用 new 分配到堆上，因为它需要跨多个异步回调存活
    auto *pathToIdMap = new QMap<QString, QString>();

    // 第一步：创建根文件夹
    QJsonObject params;
    params["name"] = rootFolderName;
    // parent_id：如果 parentFolderId 为空字符串，传 null 给后端表示根目录
    if (parentFolderId.isEmpty()) {
        params["parent_id"] = QJsonValue::Null;
    } else {
        params["parent_id"] = parentFolderId;
    }

    qDebug() << "[文件夹上传] 创建根目录:" << rootFolderName << "parent_id:" << parentFolderId;

    NetworkManager::instance()->post(
        "/files/folder", params,
        // 成功回调
        [this, result, pathToIdMap](const QJsonObject &data) {
            // 后端返回的新建文件夹 ID
            QString rootId = data["id"].toVariant().toString();
            (*pathToIdMap)[""] = rootId; // 空字符串 = 根文件夹

            qDebug() << "[文件夹上传] 根目录创建成功，服务端 ID:" << rootId;

            if (result.dirs.isEmpty()) {
                // 没有子目录，直接入队文件
                QList<QUrl> fileUrls;
                for (const auto &file : result.files) {
                    fileUrls.append(QUrl::fromLocalFile(file.first));
                }
                enqueueFiles(fileUrls, rootId);
                delete pathToIdMap;
            } else {
                // 有子目录，开始链式创建
                createDirsRecursive(result, 0, pathToIdMap, "");
            }
        },
        // 失败回调
        [pathToIdMap, rootFolderName](const QString &errMsg) {
            qWarning() << "[文件夹上传] 创建根目录失败:" << rootFolderName << "错误:" << errMsg;
            delete pathToIdMap;
        }
        );
}

void UploadController::createDirsRecursive(const FolderScanResult &result,
                                           int dirIndex,
                                           QMap<QString, QString> *pathToIdMap,
                                           const QString &parentFolderId)
{
    Q_UNUSED(parentFolderId)

    // 递归终止条件：所有子目录都已创建完毕
    if (dirIndex >= result.dirs.size()) {
        qDebug() << "[文件夹上传] 所有目录创建完毕，开始入队文件";

        // 将每个文件分配到正确的服务端目录
        for (const auto &file : result.files) {
            const QString &absolutePath = file.first;
            const QString &relativeDir  = file.second;

            // 从 map 中查找该文件所属目录的服务端 ID
            QString folderId = pathToIdMap->value(relativeDir);
            if (folderId.isEmpty()) {
                qWarning() << "[文件夹上传] 找不到目录映射:" << relativeDir << "，文件:" << absolutePath;
                continue;
            }

            QList<QUrl> singleFile;
            singleFile.append(QUrl::fromLocalFile(absolutePath));
            enqueueFiles(singleFile, folderId);
        }

        delete pathToIdMap;
        return;
    }

    // 取出当前要创建的子目录
    const auto &dirEntry = result.dirs[dirIndex];
    const QString &relativePath = dirEntry.first;  // 例如 "src/core"
    const QString &dirName      = dirEntry.second;  // 例如 "core"

    // 找到父目录的服务端 ID
    // 例如 "src/core" 的父路径是 "src"
    QString parentRelPath;
    int lastSlash = relativePath.lastIndexOf('/');
    if (lastSlash >= 0) {
        parentRelPath = relativePath.left(lastSlash);
    }
    // 如果没有斜杠，说明是顶层子目录，父目录就是根文件夹（key = ""）

    QString parentId = pathToIdMap->value(parentRelPath);
    if (parentId.isEmpty()) {
        qWarning() << "[文件夹上传] 父目录 ID 缺失:" << parentRelPath << "，跳过:" << relativePath;
        // 跳过此目录，继续下一个
        createDirsRecursive(result, dirIndex + 1, pathToIdMap, "");
        return;
    }

    QJsonObject params;
    params["name"] = dirName;
    params["parent_id"] = parentId;

    qDebug() << "[文件夹上传] 创建子目录:" << relativePath << "(" << dirName << ") parent_id:" << parentId;

    NetworkManager::instance()->post(
        "/files/folder", params,
        // 成功：记录映射，继续下一个
        [this, result, dirIndex, pathToIdMap, relativePath](const QJsonObject &data) {
            QString newId = data["id"].toVariant().toString();
            (*pathToIdMap)[relativePath] = newId;

            qDebug() << "[文件夹上传] 子目录创建成功:" << relativePath << "→ ID:" << newId;

            // 链式调用：创建下一个目录
            createDirsRecursive(result, dirIndex + 1, pathToIdMap, "");
        },
        // 失败：打印错误，但继续尝试下一个（尽力而为）
        [this, result, dirIndex, pathToIdMap, relativePath](const QString &errMsg) {
            qWarning() << "[文件夹上传] 子目录创建失败:" << relativePath << "错误:" << errMsg;
            // 继续下一个，该目录下的文件会因为找不到 ID 而被跳过
            createDirsRecursive(result, dirIndex + 1, pathToIdMap, "");
        }
        );
}

// ════════════════════════════════════════════
//  取消 / 清理
// ════════════════════════════════════════════

void UploadController::cancelTask(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0)
        return;

    UploadTask &task = m_model->tasks()[row];

    switch (task.status) {
    case UploadStatus::Queued:
        // 排队中的任务：直接从列表移除
        m_model->removeTask(taskId);
        emit pendingCountChanged();
        qDebug() << "[队列] 已移除排队中的任务:" << taskId;
        break;

    case UploadStatus::Uploading:
        // 取消正在上传的任务
        if (task.isMultipart) {
            // 分片上传：中断所有活跃分片，调用 abort API
            for (auto &reply : task.activePartReplies) {
                if (reply) {
                    reply->abort();
                    reply->deleteLater();
                }
            }
            task.activePartReplies.clear();
            if (!task.uploadId.isEmpty()) {
                abortMultipartUpload(task.uploadId);
            }
        } else if (task.reply) {
            task.reply->abort();
        }
        task.status   = UploadStatus::Canceled;
        task.errorMsg = "用户已取消";

        {
            QModelIndex idx = m_model->index(row, 0);
            emit m_model->dataChanged(idx, idx, {
                                                    UploadListModel::StatusRole,
                                                    UploadListModel::ErrorMsgRole
                                                });
        }

        releaseSlot();
        qDebug() << "[队列] 已取消正在上传的任务:" << taskId;
        break;

    case UploadStatus::Hashing:
        // 哈希计算在子线程中进行，我们无法直接中断 QtConcurrent::run
        // 但可以先标记状态：哈希完成后回到主线程时会检查状态并跳过
        task.status   = UploadStatus::Canceled;
        task.errorMsg = "用户已取消";
        {
            QModelIndex idx = m_model->index(row, 0);
            emit m_model->dataChanged(idx, idx, {
                                                    UploadListModel::StatusRole,
                                                    UploadListModel::ErrorMsgRole
                                                });
        }
        // 注意：槽位会在 startHashing 的 watcher->finished 回调中释放
        qDebug() << "[队列] 已标记哈希中的任务为取消:" << taskId;
        break;

    case UploadStatus::Paused:
        // 暂停中的任务：清理状态文件并移除
        if (task.isMultipart && !task.uploadId.isEmpty()) {
            abortMultipartUpload(task.uploadId);
        }
        m_model->removeTask(taskId);
        qDebug() << "[队列] 已移除暂停中的任务:" << taskId;
        break;

    default:
        // Success / Failed / Canceled 状态的任务不需要再取消
        qDebug() << "[队列] 任务" << taskId << "已经结束，无需取消";
        break;
    }
}

void UploadController::clearFinished()
{
    m_model->removeFinished();
}

void UploadController::cancelAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == UploadStatus::Queued ||
            task.status == UploadStatus::Hashing ||
            task.status == UploadStatus::Uploading ||
            task.status == UploadStatus::Paused) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        cancelTask(id);
    }
    m_model->removeFinished();
}

void UploadController::pauseTask(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    UploadTask &task = m_model->tasks()[row];
    if (task.status != UploadStatus::Uploading)
        return;

    // 先标记状态为暂停，防止 finished 回调误判
    m_model->updateStatus(taskId, UploadStatus::Paused);

    if (task.isMultipart) {
        // 分片上传：中断所有活跃的分片请求
        for (auto &reply : task.activePartReplies) {
            if (reply) {
                reply->abort();
                reply->deleteLater();
            }
        }
        task.activePartReplies.clear();
        task.activePartsCount = 0;
        // 已完成的分片 eTags 保持不变，JSON 状态已持久化
    } else {
        // 单文件上传：中断网络连接
        if (task.reply) {
            task.reply->abort();
            task.reply->deleteLater();
            task.reply = nullptr;
        }
    }

    releaseSlot();
    // releaseSlot 内部已调用 scheduleNext
}

void UploadController::resumeTask(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    UploadTask &task = m_model->tasks()[row];
    if (task.status != UploadStatus::Paused)
        return;

    if (task.isMultipart) {
        // 分片上传：保留 eTags，重新进入调度（会重新请求预签名 URL）
        task.presignedUrls.clear();  // 旧 URL 可能已过期
        task.status = UploadStatus::Queued;
        m_model->updateStatus(taskId, UploadStatus::Queued);
    } else {
        // 单文件上传：重置进度，从头上传
        task.receivedBytes = 0;
        task.progress = 0;
        task.status = UploadStatus::Queued;
        m_model->updateStatus(taskId, UploadStatus::Queued);
        m_model->updateProgress(taskId, 0);
    }

    emit pendingCountChanged();
    scheduleNext();
}

void UploadController::pauseAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == UploadStatus::Uploading) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        pauseTask(id);
    }
}

void UploadController::resumeAll()
{
    QStringList ids;
    for (const auto &task : m_model->tasks()) {
        if (task.status == UploadStatus::Paused) {
            ids.append(task.taskId);
        }
    }
    for (const QString &id : ids) {
        resumeTask(id);
    }
}

// ════════════════════════════════════════════
//  启动恢复：从 JSON 恢复未完成的分片上传
// ════════════════════════════════════════════

void UploadController::restorePendingUploads()
{
    QList<MultipartState> pendingStates = m_stateManager->loadAllPending();

    if (pendingStates.isEmpty()) {
        qDebug() << "[恢复] 没有未完成的分片上传任务";
        return;
    }

    qDebug() << "[恢复] 发现" << pendingStates.size() << "个未完成的分片上传任务";

    QList<UploadTask> restoredTasks;

    for (const MultipartState &state : pendingStates) {
        // 检查本地文件是否还存在
        QFileInfo fi(state.localPath);
        if (!fi.exists()) {
            qWarning() << "[恢复] 本地文件不存在，清理状态:" << state.localPath;
            m_stateManager->removeState(state.uploadId);
            continue;
        }

        // 检查文件指纹是否匹配（文件未被修改）
        QString currentFp = UploadStateManager::fingerprint(state.localPath);
        if (currentFp != state.fingerprint) {
            qWarning() << "[恢复] 文件已被修改，清理状态:" << state.localPath;
            m_stateManager->removeState(state.uploadId);
            // 通知后端中止旧的上传
            abortMultipartUpload(state.uploadId);
            continue;
        }

        UploadTask task;
        task.taskId         = QUuid::createUuid().toString(QUuid::WithoutBraces);
        task.localPath      = state.localPath;
        task.fileName       = fi.fileName();
        task.fileSize       = state.fileSize;
        task.targetFolderId = state.targetFolderId;
        task.sha256         = state.fileHash;
        task.status         = UploadStatus::Paused;  // 以暂停状态恢复，等用户手动恢复
        task.isMultipart    = true;
        task.uploadId       = state.uploadId;
        task.partSize       = state.partSize;
        task.totalParts     = state.totalParts;
        task.eTags          = state.completedParts;
        task.completedPartsCount = state.completedParts.size();

        // 计算已上传进度
        qint64 completedBytes = static_cast<qint64>(task.completedPartsCount) * task.partSize;
        if (completedBytes > task.fileSize) completedBytes = task.fileSize;
        task.receivedBytes = completedBytes;
        task.progress = static_cast<int>(completedBytes * 100 / task.fileSize);

        restoredTasks.append(task);

        qDebug() << "[恢复] 任务:" << task.fileName
                 << "已完成:" << task.completedPartsCount << "/" << task.totalParts;
    }

    if (!restoredTasks.isEmpty()) {
        m_model->appendTasks(restoredTasks);
        emit pendingCountChanged();
    }
}

// ════════════════════════════════════════════
//  调度器：限流核心
// ════════════════════════════════════════════

void UploadController::scheduleNext()
{
    const auto &tasks = m_model->tasks();

    while (m_activeCount < m_maxConcurrent) {
        // 寻找第一个处于 Queued 状态的任务
        QString nextTaskId;
        for (const auto &task : tasks) {
            if (task.status == UploadStatus::Queued) {
                nextTaskId = task.taskId;
                break;
            }
        }

        if (nextTaskId.isEmpty())
            break; // 没有排队中的任务了

        // 占用一个并发槽位
        m_activeCount++;
        emit activeCountChanged();
        emit pendingCountChanged();

        // 开始阶段 1：计算哈希
        startHashing(nextTaskId);
    }
}

// ════════════════════════════════════════════
//  阶段 1：子线程计算哈希
// ════════════════════════════════════════════

void UploadController::startHashing(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) {
        releaseSlot();
        return;
    }

    UploadTask &task = m_model->tasks()[row];

    // 更新状态为 Hashing（精准刷新 StatusRole）
    m_model->updateStatus(taskId, UploadStatus::Hashing);

    QString filePath = task.localPath;
    qDebug() << "[哈希] 开始计算:" << task.fileName;

    // 使用 QtConcurrent 将耗时的文件读取 + SHA-256 计算抛到子线程
    QFuture<QString> future = QtConcurrent::run([filePath]() -> QString {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }

        QCryptographicHash hasher(QCryptographicHash::Sha256);

        // 分块读取（每次 1MB），避免一次性把大文件全部装入内存
        constexpr qint64 CHUNK_SIZE = 1024 * 1024;
        while (!file.atEnd()) {
            hasher.addData(file.read(CHUNK_SIZE));
        }

        return hasher.result().toHex();
    });

    // 使用 QFutureWatcher 在主线程中接收子线程的结果
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, taskId]() {
        QString hash = watcher->result();
        watcher->deleteLater();

        int row = m_model->findByTaskId(taskId);
        if (row < 0) {
            // 任务已被移除，释放槽位
            releaseSlot();
            return;
        }

        UploadTask &task = m_model->tasks()[row];

        // ── 检查：哈希期间用户是否已经取消了这个任务 ──
        if (task.status == UploadStatus::Canceled) {
            qDebug() << "[哈希] 任务已被用户取消，跳过:" << task.fileName;
            releaseSlot();
            return;
        }

        if (hash.isEmpty()) {
            // 哈希计算失败
            m_model->updateError(taskId, "无法读取文件");
            emit taskFailed(taskId, task.fileName, "无法读取文件");
            releaseSlot();
            return;
        }

        task.sha256 = hash;
        qDebug() << "[哈希] 计算完成:" << task.fileName << "SHA256:" << hash.left(16) + "...";

        // 哈希完成，进入阶段 2：发起网络上传
        startUpload(taskId);
    });

    watcher->setFuture(future);
}

// ════════════════════════════════════════════
//  阶段 2：发起网络上传
// ════════════════════════════════════════════

void UploadController::startUpload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) {
        releaseSlot();
        return;
    }

    UploadTask &task = m_model->tasks()[row];

    // 分支：小文件走原始单文件上传，大文件走分片上传
    const qint64 MULTIPART_THRESHOLD = 5 * 1024 * 1024;  // 5MB
    if (task.fileSize >= MULTIPART_THRESHOLD || task.isMultipart) {
        // 断点续传时 isMultipart 已经为 true，不重复计算
        if (!task.isMultipart) {
            task.isMultipart = true;
            task.partSize = 8 * 1024 * 1024;  // 8MB
            task.totalParts = static_cast<int>((task.fileSize + task.partSize - 1) / task.partSize);
        }
        initMultipartUpload(taskId);
        return;
    }

    // ── 小文件：原始单文件上传流程 ──
    m_model->updateStatus(taskId, UploadStatus::Uploading);
    m_model->updateProgress(taskId, 0);

    QString fileName = task.fileName;
    qDebug() << "[上传] 网络请求开始:" << fileName << "到目录:" << task.targetFolderId;

    QNetworkReply *reply = NetworkManager::instance()->uploadFile(
        "/files/upload",
        task.localPath,
        task.targetFolderId,
        task.sha256,

        // 回调 1: 进度更新 (Progress)
        [this, taskId](qint64 sent, qint64 total) {
            if (total > 0) {
                int percent = static_cast<int>(sent * 100 / total);
                m_model->updateProgress(taskId, percent);

                // 跟踪已上传字节数
                int currentRow = m_model->findByTaskId(taskId);
                if (currentRow >= 0) {
                    m_model->tasks()[currentRow].receivedBytes = sent;
                    QModelIndex idx = m_model->index(currentRow, 0);
                    emit m_model->dataChanged(idx, idx, {UploadListModel::ReceivedBytesRole});
                }
            }
        },

        // 回调 2: 成功处理 (Success)
        [this, taskId, fileName](const QJsonObject &data) {
            // 安全检查：防止在长达几分钟的上传过程中，用户取消了任务
            int currentRow = m_model->findByTaskId(taskId);
            if (currentRow >= 0 && m_model->tasks()[currentRow].status == UploadStatus::Canceled) {
                releaseSlot(); // 如果已取消，直接释放槽位走人
                return;
            }

            m_model->updateStatus(taskId, UploadStatus::Success);
            m_model->updateProgress(taskId, 100);
            emit taskSuccess(taskId, fileName);

            releaseSlot(); // ← 完成后才释放槽位让下一个任务进场
        },

        // 回调 3: 失败或取消处理 (Error)
        [this, taskId, fileName](const QString &errMsg) {
            int currentRow = m_model->findByTaskId(taskId);
            if (currentRow >= 0) {
                // 如果是用户主动 abort 导致的错误，状态已被 cancelTask/pauseTask 置为 Canceled/Paused，不用改回 Failed
                if (m_model->tasks()[currentRow].status != UploadStatus::Canceled &&
                    m_model->tasks()[currentRow].status != UploadStatus::Paused) {
                    m_model->updateStatus(taskId, UploadStatus::Failed);
                    m_model->updateError(taskId, errMsg);
                    emit taskFailed(taskId, fileName, errMsg);
                }
            }

            releaseSlot(); // 失败也要释放并发槽位
        }
        );

    // 3. 保存 reply 句柄，用于支持“取消上传”的“刹车”操作
    row = m_model->findByTaskId(taskId);
    if (row >= 0) {
        m_model->tasks()[row].reply = reply;
    } else if (reply) {
        // 极端边缘情况：刚发完请求任务就被清除了
        reply->abort();
        reply->deleteLater();
    }
}

// ════════════════════════════════════════════
//  辅助
// ════════════════════════════════════════════

void UploadController::releaseSlot()
{
    m_activeCount--;
    emit activeCountChanged();
    emit pendingCountChanged();

    // 释放槽位后立刻检查队列中是否有等待的任务
    scheduleNext();
}

// ════════════════════════════════════════════
//  分片上传 阶段 1：初始化（获取 upload_id + 预签名 URL）
// ════════════════════════════════════════════

void UploadController::initMultipartUpload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) { releaseSlot(); return; }

    UploadTask &task = m_model->tasks()[row];

    m_model->updateStatus(taskId, UploadStatus::Uploading);

    // 断点续传时，恢复已有进度
    if (task.completedPartsCount > 0) {
        qint64 completedBytes = static_cast<qint64>(task.completedPartsCount) * task.partSize;
        if (completedBytes > task.fileSize) completedBytes = task.fileSize;
        task.receivedBytes = completedBytes;
        int percent = static_cast<int>(completedBytes * 100 / task.fileSize);
        m_model->updateProgress(taskId, percent);
    } else {
        m_model->updateProgress(taskId, 0);
    }

    qDebug() << "[分片上传] 初始化:" << task.fileName
             << "总大小:" << task.fileSize
             << "分片数:" << task.totalParts;

    // 构建初始化请求参数
    QJsonObject params;
    params["file_name"]   = task.fileName;
    params["file_size"]   = task.fileSize;
    params["total_parts"] = task.totalParts;
    params["file_hash"]   = task.sha256;
    params["parent_id"]   = task.targetFolderId;

    // 断点续传时只传 upload_id，后端自行调 S3 list_parts 溯源分片状态
    if (!task.uploadId.isEmpty()) {
        params["upload_id"] = task.uploadId;
    }

    QString fileName = task.fileName;

    NetworkManager::instance()->post(
        "/files/multipart/init", params,

        // 成功回调
        [this, taskId, fileName](const QJsonObject &data) {
            int row = m_model->findByTaskId(taskId);
            if (row < 0) { releaseSlot(); return; }

            UploadTask &task = m_model->tasks()[row];

            // 检查是否被取消/暂停
            if (task.status != UploadStatus::Uploading) return;

            // ── 秒传检测：后端查到 file_hash 已存在的文件 ──
            QString status = data["status"].toString();
            if (status == "fast_uploaded") {
                qDebug() << "[分片上传] 秒传命中:" << fileName;

                m_model->updateStatus(taskId, UploadStatus::Success);
                m_model->updateProgress(taskId, 100);

                task.receivedBytes = task.fileSize;
                QModelIndex idx = m_model->index(row, 0);
                emit m_model->dataChanged(idx, idx, {UploadListModel::ReceivedBytesRole});

                emit taskSuccess(taskId, fileName);
                releaseSlot();
                return;
            }

            // ── 正常分片上传 / 断点续传 ──
            task.uploadId = data["upload_id"].toString();

            // 解析预签名 URL 列表（后端已通过 list_parts 排除了已完成的分片）
            task.presignedUrls.clear();
            QJsonArray urlsArr = data["presigned_urls"].toArray();
            for (const auto &v : urlsArr) {
                QJsonObject urlObj = v.toObject();
                int partNumber = urlObj["part_number"].toInt();
                QString url = urlObj["url"].toString();
                task.presignedUrls[partNumber] = url;
            }

            // 从后端 list_parts 结果同步服务端已确认的已完成分片
            // 后端返回 completed_parts: [{part_number, etag}, ...]
            if (data.contains("completed_parts")) {
                task.eTags.clear();
                QJsonArray partsArr = data["completed_parts"].toArray();
                for (const auto &v : partsArr) {
                    QJsonObject p = v.toObject();
                    task.eTags.append({p["part_number"].toInt(), p["etag"].toString()});
                }
                task.completedPartsCount = task.eTags.size();
            }

            qDebug() << "[分片上传] 初始化成功:"
                     << "upload_id=" << task.uploadId
                     << "已完成:" << task.completedPartsCount
                     << "待上传:" << task.presignedUrls.size();

            // 持久化状态
            MultipartState state;
            state.uploadId       = task.uploadId;
            state.fingerprint    = UploadStateManager::fingerprint(task.localPath);
            state.localPath      = task.localPath;
            state.targetFolderId = task.targetFolderId;
            state.fileHash       = task.sha256;
            state.fileSize       = task.fileSize;
            state.partSize       = task.partSize;
            state.totalParts     = task.totalParts;
            state.completedParts = task.eTags;
            state.createdAt      = QDateTime::currentDateTime();
            m_stateManager->saveState(state);

            // 开始上传分片
            uploadNextParts(taskId);
        },

        // 失败回调
        [this, taskId](const QString &errMsg) {
            int row = m_model->findByTaskId(taskId);
            if (row >= 0) {
                UploadTask &task = m_model->tasks()[row];
                if (task.status == UploadStatus::Uploading) {
                    m_model->updateError(taskId, "分片初始化失败: " + errMsg);
                    emit taskFailed(taskId, task.fileName, errMsg);
                }
            }
            releaseSlot();
        }
        );
}

// ════════════════════════════════════════════
//  分片上传 阶段 2：滑动窗口调度
// ════════════════════════════════════════════

void UploadController::uploadNextParts(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    UploadTask &task = m_model->tasks()[row];

    // 如果不在上传状态（被暂停/取消），不调度
    if (task.status != UploadStatus::Uploading)
        return;

    // 检查是否所有分片都已完成
    if (task.completedPartsCount >= task.totalParts) {
        completeMultipartUpload(taskId);
        return;
    }

    // 收集已完成的 partNumber
    QSet<int> completedSet;
    for (const auto &p : task.eTags) {
        completedSet.insert(p.first);
    }

    // 收集已经在上传的 partNumber（通过 presignedUrls 剩余的来推断）
    // 滑动窗口：补充到 m_maxPartsPerTask 个并发
    QList<int> partsToUpload;
    for (auto it = task.presignedUrls.constBegin();
         it != task.presignedUrls.constEnd(); ++it) {
        if (task.activePartsCount >= m_maxPartsPerTask)
            break;
        int partNum = it.key();
        if (!completedSet.contains(partNum)) {
            partsToUpload.append(partNum);
            task.activePartsCount++;
        }
    }

    for (int partNum : partsToUpload) {
        uploadSinglePart(taskId, partNum);
    }
}

// ════════════════════════════════════════════
//  分片上传 阶段 3：上传单个分片
// ════════════════════════════════════════════

static constexpr int MAX_PART_RETRIES = 3;
static constexpr int RETRY_BASE_DELAY_MS = 500;

void UploadController::uploadSinglePart(const QString &taskId, int partNumber)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    UploadTask &task = m_model->tasks()[row];

    if (task.status != UploadStatus::Uploading)
        return;

    // 从本地文件读取分片数据
    QFile file(task.localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[分片上传] 无法打开文件:" << task.localPath;
        m_model->updateError(taskId, "无法打开本地文件");
        emit taskFailed(taskId, task.fileName, "无法打开本地文件");
        releaseSlot();
        return;
    }

    qint64 offset = static_cast<qint64>(partNumber - 1) * task.partSize;
    file.seek(offset);

    // 最后一片可能小于 partSize
    qint64 chunkSize = qMin(task.partSize, task.fileSize - offset);
    QByteArray chunkData = file.read(chunkSize);
    file.close();

    if (chunkData.size() != chunkSize) {
        qWarning() << "[分片上传] 读取分片失败: part" << partNumber
                   << "期望:" << chunkSize << "实际:" << chunkData.size();
        task.activePartsCount--;
        return;
    }

    // 获取并消费预签名 URL（take = get + remove，防止 uploadNextParts 重复调度）
    QString presignedUrl = task.presignedUrls.take(partNumber);
    if (presignedUrl.isEmpty()) {
        qWarning() << "[分片上传] 没有分片" << partNumber << "的预签名URL";
        task.activePartsCount--;
        return;
    }

    qDebug() << "[分片上传] 上传分片:" << partNumber << "/" << task.totalParts
             << "大小:" << chunkData.size();

    // PUT 到预签名 URL
    QNetworkReply *reply = NetworkManager::instance()->putRawData(
        QUrl(presignedUrl), chunkData,
        // 进度回调：更新该分片的进度
        [this, taskId, partNumber](qint64 sent, qint64 total) {
            Q_UNUSED(total)
            int row = m_model->findByTaskId(taskId);
            if (row < 0) return;

            UploadTask &task = m_model->tasks()[row];

            // 计算总进度：已完成分片的字节 + 当前分片已发送字节
            qint64 completedBytes = static_cast<qint64>(task.completedPartsCount) * task.partSize;
            // 修正最后一片
            if (task.completedPartsCount > 0 && task.completedPartsCount == task.totalParts) {
                completedBytes = task.fileSize;
            }
            completedBytes += sent;
            if (completedBytes > task.fileSize) completedBytes = task.fileSize;

            task.receivedBytes = completedBytes;
            int percent = static_cast<int>(completedBytes * 100 / task.fileSize);
            m_model->updateProgress(taskId, percent);

            QModelIndex idx = m_model->index(row, 0);
            emit m_model->dataChanged(idx, idx, {UploadListModel::ReceivedBytesRole});
        }
        );

    // 保存 reply 句柄
    task.activePartReplies.append(reply);

    // 监听完成信号
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, taskId, partNumber]() {
                reply->deleteLater();

                int row = m_model->findByTaskId(taskId);
                if (row < 0) return;

                UploadTask &task = m_model->tasks()[row];

                // 从活跃列表移除
                task.activePartReplies.removeAll(reply);
                task.activePartsCount--;

                // 如果被暂停/取消，不继续处理
                if (task.status != UploadStatus::Uploading) return;

                // 检查错误
                if (reply->error() != QNetworkReply::NoError) {
                    if (reply->error() == QNetworkReply::OperationCanceledError) {
                        return;  // 暂停/取消导致的中断
                    }

                    qWarning() << "[分片上传] 分片" << partNumber
                               << "上传失败:" << reply->errorString();

                    // 重试逻辑：最多重试 MAX_PART_RETRIES 次，指数退避
                    int retryCount = reply->property("retryCount").toInt();
                    if (retryCount < MAX_PART_RETRIES) {
                        int delayMs = RETRY_BASE_DELAY_MS * (1 << retryCount);
                        qWarning() << "[分片上传] 分片" << partNumber
                                   << "将在" << delayMs << "ms后重试 (第" << (retryCount + 1)
                                   << "/" << MAX_PART_RETRIES << "次)";
                        QTimer::singleShot(delayMs, this, [this, taskId, partNumber, reply, retryCount]() {
                            reply->setProperty("retryCount", retryCount + 1);
                            uploadSinglePart(taskId, partNumber);
                        });
                        return;
                    }

                    m_model->updateError(taskId, "分片 " + QString::number(partNumber)
                                                     + " 上传失败: " + reply->errorString());
                    emit taskFailed(taskId, task.fileName, reply->errorString());
                    releaseSlot();
                    return;
                }

                // 提取 ETag
                QString etag = QString::fromLatin1(reply->rawHeader("ETag"));
                if (etag.isEmpty()) {
                    etag = QString::fromLatin1(reply->rawHeader("etag"));
                }

                qDebug() << "[分片上传] 分片完成:" << partNumber
                         << "ETag:" << etag;

                onPartCompleted(taskId, partNumber, etag);
            });
}

// ════════════════════════════════════════════
//  分片上传 阶段 4：分片完成处理
// ════════════════════════════════════════════

void UploadController::onPartCompleted(const QString &taskId, int partNumber, const QString &etag)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) return;

    UploadTask &task = m_model->tasks()[row];

    // 记录 ETag
    task.eTags.append({partNumber, etag});
    task.completedPartsCount = task.eTags.size();

    // 更新进度
    qint64 completedBytes = static_cast<qint64>(task.completedPartsCount) * task.partSize;
    if (completedBytes > task.fileSize) completedBytes = task.fileSize;
    task.receivedBytes = completedBytes;
    int percent = static_cast<int>(completedBytes * 100 / task.fileSize);
    m_model->updateProgress(taskId, percent);

    QModelIndex idx = m_model->index(row, 0);
    emit m_model->dataChanged(idx, idx, {UploadListModel::ReceivedBytesRole});

    // 持久化到 JSON
    MultipartState state;
    state.uploadId       = task.uploadId;
    state.fingerprint    = UploadStateManager::fingerprint(task.localPath);
    state.localPath      = task.localPath;
    state.targetFolderId = task.targetFolderId;
    state.fileHash       = task.sha256;
    state.fileSize       = task.fileSize;
    state.partSize       = task.partSize;
    state.totalParts     = task.totalParts;
    state.completedParts = task.eTags;
    state.createdAt      = QDateTime::currentDateTime();
    m_stateManager->saveState(state);

    qDebug() << "[分片上传] 进度:" << task.completedPartsCount
             << "/" << task.totalParts
             << "(" << percent << "%)";

    // 调度更多分片或完成合并
    uploadNextParts(taskId);
}

// ════════════════════════════════════════════
//  分片上传 阶段 5：合并提交
// ════════════════════════════════════════════

void UploadController::completeMultipartUpload(const QString &taskId)
{
    int row = m_model->findByTaskId(taskId);
    if (row < 0) { releaseSlot(); return; }

    UploadTask &task = m_model->tasks()[row];

    qDebug() << "[分片上传] 所有分片完成，发起合并:" << task.uploadId;

    // 组装 ETag 列表
    QJsonArray partsArr;
    for (const auto &p : task.eTags) {
        QJsonObject partObj;
        partObj["part_number"] = p.first;
        partObj["etag"]        = p.second;
        partsArr.append(partObj);
    }

    QJsonObject params;
    params["upload_id"] = task.uploadId;
    params["parts"]     = partsArr;

    QString fileName = task.fileName;

    NetworkManager::instance()->post(
        "/files/multipart/complete", params,

        // 成功
        [this, taskId, fileName](const QJsonObject &data) {
            Q_UNUSED(data)
            int row = m_model->findByTaskId(taskId);
            if (row < 0) { releaseSlot(); return; }

            UploadTask &task = m_model->tasks()[row];

            m_model->updateStatus(taskId, UploadStatus::Success);
            m_model->updateProgress(taskId, 100);

            // 更新 receivedBytes 为完整文件大小
            task.receivedBytes = task.fileSize;
            QModelIndex idx = m_model->index(row, 0);
            emit m_model->dataChanged(idx, idx, {UploadListModel::ReceivedBytesRole});

            emit taskSuccess(taskId, fileName);

            // 清理持久化状态
            m_stateManager->removeState(task.uploadId);

            qDebug() << "[分片上传] 合并成功:" << fileName;

            releaseSlot();
        },

        // 失败
        [this, taskId, fileName](const QString &errMsg) {
            int row = m_model->findByTaskId(taskId);
            if (row >= 0) {
                m_model->updateError(taskId, "合并失败: " + errMsg);
                emit taskFailed(taskId, fileName, errMsg);
            }
            releaseSlot();
        }
        );
}

// ════════════════════════════════════════════
//  分片上传 清理：中止上传 + 删除状态
// ════════════════════════════════════════════

void UploadController::abortMultipartUpload(const QString &uploadId)
{
    if (uploadId.isEmpty()) return;

    qDebug() << "[分片上传] 中止上传:" << uploadId;

    // 清理本地状态文件
    m_stateManager->removeState(uploadId);

    // 通知服务端清理已上传的分片
    NetworkManager::instance()->deleteResource(
        "/files/multipart/abort?upload_id=" + uploadId,
        [uploadId](const QJsonObject &) {
            qDebug() << "[分片上传] 服务端分片已清理:" << uploadId;
        },
        [uploadId](const QString &errMsg) {
            qWarning() << "[分片上传] 服务端分片清理失败:" << uploadId
                       << "错误:" << errMsg;
        }
        );
}
