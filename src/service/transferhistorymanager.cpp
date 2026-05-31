#include "transferhistorymanager.h"
#include "userstoragepaths.h"
#include "../controller/uploadcontroller.h"
#include "../controller/downloadcontroller.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>


// ═══════════════════════ TransferHistoryModel ═══════════════════════

TransferHistoryModel::TransferHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TransferHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_records.size();
}

QVariant TransferHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_records.size())
        return {};

    const auto &r = m_records.at(index.row());
    switch (role) {
    case TaskIdRole:     return r.taskId;
    case FileNameRole:   return r.fileName;
    case FileSizeRole:   return r.fileSize;
    case IsUploadRole:   return r.isUpload;
    case StatusRole:     return r.status;
    case ErrorMsgRole:   return r.errorMsg;
    case FinishedAtRole: return r.finishedAt.toString("yyyy-MM-dd hh:mm");
    }
    return {};
}

QHash<int, QByteArray> TransferHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TaskIdRole]     = "taskId";
    roles[FileNameRole]   = "fileName";
    roles[FileSizeRole]   = "fileSize";
    roles[IsUploadRole]   = "isUpload";
    roles[StatusRole]     = "status";
    roles[ErrorMsgRole]   = "errorMsg";
    roles[FinishedAtRole] = "finishedAt";
    return roles;
}

void TransferHistoryModel::appendRecord(const TransferRecord &record)
{
    // 插入到顶部，最新的在最前面
    beginInsertRows(QModelIndex(), 0, 0);
    m_records.prepend(record);
    endInsertRows();
    emit countChanged();
}

void TransferHistoryModel::clear()
{
    if (m_records.isEmpty()) return;
    beginResetModel();
    m_records.clear();
    endResetModel();
    emit countChanged();
}


// ═══════════════════════ TransferHistoryManager ═══════════════════════

TransferHistoryManager::TransferHistoryManager(QObject *parent)
    : QObject(parent)
{
    m_model = new TransferHistoryModel(this);
    loadFromDisk();
}

TransferHistoryManager* TransferHistoryManager::instance()
{
    static TransferHistoryManager inst;
    return &inst;
}

TransferHistoryModel* TransferHistoryManager::model() const
{
    return m_model;
}

void TransferHistoryManager::connectControllers()
{
    auto *upload = UploadController::instance();
    auto *download = DownloadController::instance();

    // 上传成功
    connect(upload, &UploadController::taskSuccess, this,
            [this, upload](const QString &taskId, const QString &fileName) {
        // 尝试从 model 中获取文件大小
        qint64 fileSize = 0;
        auto *model = upload->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            auto idx = model->index(i, 0);
            if (model->data(idx, 257).toString() == taskId) { // TaskIdRole = 257
                fileSize = model->data(idx, 259).toLongLong(); // FileSizeRole = 259
                break;
            }
        }
        addRecord(taskId, fileName, fileSize, true, 3);
    });

    // 上传失败
    connect(upload, &UploadController::taskFailed, this,
            [this, upload](const QString &taskId, const QString &fileName, const QString &errorMsg) {
        qint64 fileSize = 0;
        auto *model = upload->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            auto idx = model->index(i, 0);
            if (model->data(idx, 257).toString() == taskId) {
                fileSize = model->data(idx, 259).toLongLong();
                break;
            }
        }
        addRecord(taskId, fileName, fileSize, true, 4, errorMsg);
    });

    // 下载成功
    connect(download, &DownloadController::taskSuccess, this,
            [this, download](const QString &taskId, const QString &fileName) {
        qint64 totalBytes = 0;
        auto *model = download->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            auto idx = model->index(i, 0);
            if (model->data(idx, 257).toString() == taskId) { // TaskIdRole
                totalBytes = model->data(idx, 259).toLongLong(); // TotalBytesRole = 259
                break;
            }
        }
        addRecord(taskId, fileName, totalBytes, false, 3);
    });

    // 下载失败
    connect(download, &DownloadController::taskFailed, this,
            [this, download](const QString &taskId, const QString &fileName, const QString &errorMsg) {
        qint64 totalBytes = 0;
        auto *model = download->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            auto idx = model->index(i, 0);
            if (model->data(idx, 257).toString() == taskId) {
                totalBytes = model->data(idx, 259).toLongLong();
                break;
            }
        }
        addRecord(taskId, fileName, totalBytes, false, 4, errorMsg);
    });
}

void TransferHistoryManager::addRecord(const QString &taskId,
                                        const QString &fileName,
                                        qint64 fileSize,
                                        bool isUpload,
                                        int status,
                                        const QString &errorMsg)
{
    TransferRecord record;
    record.taskId     = taskId;
    record.fileName   = fileName;
    record.fileSize   = fileSize;
    record.isUpload   = isUpload;
    record.status     = status;
    record.errorMsg   = errorMsg;
    record.finishedAt = QDateTime::currentDateTime();

    m_model->appendRecord(record);
    saveToDisk();
}

void TransferHistoryManager::clearAll()
{
    m_model->clear();
    saveToDisk();
}

void TransferHistoryManager::reloadForCurrentUser()
{
    m_model->clear();
    loadFromDisk();
}

// ── 持久化 ──

QString TransferHistoryManager::historyFilePath() const
{
    return UserStoragePaths::userDataPath("transfer_history.json");
}

void TransferHistoryManager::loadFromDisk()
{
    QFile file(historyFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return;

    QJsonArray arr = doc.array();
    // 文件中最新的在前面，按顺序 append 到 model
    for (const auto &val : arr) {
        if (val.isObject()) {
            TransferRecord r = TransferRecord::fromJson(val.toObject());
            // 直接添加到末尾保持文件中的顺序（最新在前）
            m_model->appendRecord(r);
        }
    }
}

void TransferHistoryManager::saveToDisk()
{
    QJsonArray arr;
    for (const auto &r : m_model->records()) {
        arr.append(r.toJson());
    }

    QFile file(historyFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    file.close();
}
