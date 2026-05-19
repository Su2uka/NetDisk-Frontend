#include "uploadlistmodel.h"

UploadListModel::UploadListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

// ── QAbstractListModel 三大核心虚函数 ──

int UploadListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tasks.count();
}

QVariant UploadListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.count())
        return QVariant();

    const UploadTask &task = m_tasks[index.row()];

    switch (role) {
    case TaskIdRole:   return task.taskId;
    case FileNameRole: return task.fileName;
    case FileSizeRole: return task.fileSize;
    case ReceivedBytesRole: return task.receivedBytes;
    case StatusRole:   return static_cast<int>(task.status);
    case ProgressRole: return task.progress;
    case ErrorMsgRole: return task.errorMsg;
    }

    return QVariant();
}

QHash<int, QByteArray> UploadListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TaskIdRole]   = "taskId";
    roles[FileNameRole] = "fileName";
    roles[FileSizeRole] = "fileSize";
    roles[ReceivedBytesRole] = "receivedBytes";
    roles[StatusRole]   = "status";
    roles[ProgressRole] = "progress";
    roles[ErrorMsgRole] = "errorMsg";
    return roles;
}

// ── 精准更新接口 ──

void UploadListModel::appendTasks(const QList<UploadTask> &tasks)
{
    if (tasks.isEmpty())
        return;

    // 通知视图："我要在末尾插入 N 行"
    int first = m_tasks.count();
    int last  = first + tasks.count() - 1;
    beginInsertRows(QModelIndex(), first, last);

    m_tasks.append(tasks);

    endInsertRows();
    emit countChanged();
}

void UploadListModel::updateStatus(const QString &taskId, UploadStatus status)
{
    int row = findByTaskId(taskId);
    if (row < 0)
        return;

    m_tasks[row].status = status;

    // 精准刷新：只通知 QML 这一行的 StatusRole 变了
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {StatusRole});
}

void UploadListModel::updateProgress(const QString &taskId, int percent)
{
    int row = findByTaskId(taskId);
    if (row < 0)
        return;

    m_tasks[row].progress = percent;

    // 精准刷新：只通知 QML 这一行的 ProgressRole 变了
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ProgressRole});
}

void UploadListModel::updateError(const QString &taskId, const QString &errorMsg)
{
    int row = findByTaskId(taskId);
    if (row < 0)
        return;

    m_tasks[row].status   = UploadStatus::Failed;
    m_tasks[row].errorMsg = errorMsg;

    // 同时刷新两个角色
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {StatusRole, ErrorMsgRole});
}

void UploadListModel::removeTask(const QString &taskId)
{
    int row = findByTaskId(taskId);
    if (row < 0)
        return;

    // 通知视图："我要移除第 row 行"
    beginRemoveRows(QModelIndex(), row, row);
    m_tasks.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void UploadListModel::removeFinished()
{
    // 从后往前移除，避免索引错位
    for (int i = m_tasks.count() - 1; i >= 0; --i) {
        UploadStatus s = m_tasks[i].status;
        if (s == UploadStatus::Success || s == UploadStatus::Failed || s == UploadStatus::Canceled) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
        }
    }
    emit countChanged();
}

// ── 底层数据访问 ──

QList<UploadTask>& UploadListModel::tasks()
{
    return m_tasks;
}

const QList<UploadTask>& UploadListModel::tasks() const
{
    return m_tasks;
}

int UploadListModel::findByTaskId(const QString &taskId) const
{
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_tasks[i].taskId == taskId)
            return i;
    }
    return -1;
}
