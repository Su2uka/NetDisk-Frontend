#include "downloadlistmodel.h"

DownloadListModel::DownloadListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DownloadListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tasks.count();
}

QVariant DownloadListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.count())
        return QVariant();

    const DownloadTask &task = m_tasks[index.row()];

    switch (role) {
    case TaskIdRole:        return task.taskId;
    case FileNameRole:      return task.fileName;
    case TotalBytesRole:    return task.totalBytes;
    case ReceivedBytesRole: return task.receivedBytes;
    case ProgressRole:      return task.progress;
    case StatusRole:        return static_cast<int>(task.status);
    case ErrorMsgRole:      return task.errorMsg;
    }

    return QVariant();
}

QHash<int, QByteArray> DownloadListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TaskIdRole]        = "taskId";
    roles[FileNameRole]      = "fileName";
    roles[TotalBytesRole]    = "totalBytes";
    roles[ReceivedBytesRole] = "receivedBytes";
    roles[ProgressRole]      = "progress";
    roles[StatusRole]        = "status";
    roles[ErrorMsgRole]      = "errorMsg";
    return roles;
}

// ── 追加单个任务 ──
void DownloadListModel::appendTask(const DownloadTask &task)
{
    beginInsertRows(QModelIndex(), m_tasks.count(), m_tasks.count());
    m_tasks.append(task);
    endInsertRows();
    emit countChanged();
}

// ── 追加一批任务 ──
void DownloadListModel::appendTasks(const QList<DownloadTask> &tasks)
{
    if (tasks.isEmpty())
        return;

    beginInsertRows(QModelIndex(), m_tasks.count(), m_tasks.count() + tasks.count() - 1);
    m_tasks.append(tasks);
    endInsertRows();
    emit countChanged();
}

// ── 精准更新进度（局部刷新 ReceivedBytesRole + ProgressRole）──
void DownloadListModel::updateProgress(const QString &taskId, qint64 receivedBytes, qint64 totalBytes, int percent)
{
    int row = findByTaskId(taskId);
    if (row < 0) return;

    m_tasks[row].receivedBytes = receivedBytes;
    m_tasks[row].totalBytes = totalBytes;
    m_tasks[row].progress = percent;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ReceivedBytesRole, TotalBytesRole, ProgressRole});
}

// ── 精准更新状态 ──
void DownloadListModel::updateStatus(const QString &taskId, DownloadStatus status)
{
    int row = findByTaskId(taskId);
    if (row < 0) return;

    m_tasks[row].status = status;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {StatusRole});
}

// ── 精准更新错误信息 ──
void DownloadListModel::updateError(const QString &taskId, const QString &errorMsg)
{
    int row = findByTaskId(taskId);
    if (row < 0) return;

    m_tasks[row].status = DownloadStatus::Failed;
    m_tasks[row].errorMsg = errorMsg;

    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {StatusRole, ErrorMsgRole});
}

// ── 移除单个任务 ──
void DownloadListModel::removeTask(const QString &taskId)
{
    int row = findByTaskId(taskId);
    if (row < 0) return;

    beginRemoveRows(QModelIndex(), row, row);
    m_tasks.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

// ── 移除所有已完成 / 已失败的任务 ──
void DownloadListModel::removeFinished()
{
    for (int i = m_tasks.count() - 1; i >= 0; --i) {
        auto s = m_tasks[i].status;
        if (s == DownloadStatus::Success || s == DownloadStatus::Failed) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.removeAt(i);
            endRemoveRows();
        }
    }
    emit countChanged();
}

// ── 直接访问底层数据 ──
QList<DownloadTask>& DownloadListModel::tasks()
{
    return m_tasks;
}

const QList<DownloadTask>& DownloadListModel::tasks() const
{
    return m_tasks;
}

// ── 按 taskId 查找索引 ──
int DownloadListModel::findByTaskId(const QString &taskId) const
{
    for (int i = 0; i < m_tasks.count(); ++i) {
        if (m_tasks[i].taskId == taskId)
            return i;
    }
    return -1;
}
