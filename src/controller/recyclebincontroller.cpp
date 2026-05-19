#include "recyclebincontroller.h"
#include "../network/networkmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDebug>

// ═══════════════════════════════════════════════
//  RecycleBinModel
// ═══════════════════════════════════════════════

RecycleBinModel::RecycleBinModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RecycleBinModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.count();
}

QVariant RecycleBinModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
        return QVariant();

    const RecycleBinItem &item = m_items[index.row()];
    switch (role) {
    case FileIdRole:      return item.fileId;
    case FileNameRole:    return item.fileName;
    case FileIconRole:    return item.fileIcon;
    case IsFolderRole:    return item.isFolder;
    case IsSelectedRole:  return item.isSelected;
    case FileSizeRole:    return item.fileSize;
    case FileSizeStrRole: return item.fileSizeStr;
    case DeletedAtRole:   return item.deletedAt;
    }
    return QVariant();
}

QHash<int, QByteArray> RecycleBinModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FileIdRole]      = "fileId";
    roles[FileNameRole]    = "fileName";
    roles[FileIconRole]    = "fileIcon";
    roles[IsFolderRole]    = "isFolder";
    roles[IsSelectedRole]  = "selected";
    roles[FileSizeRole]    = "fileSize";
    roles[FileSizeStrRole] = "fileSizeStr";
    roles[DeletedAtRole]   = "deletedAt";
    return roles;
}

void RecycleBinModel::loadData(const QVariantList &data)
{
    beginResetModel();
    m_items.clear();
    for (const QVariant &var : data) {
        QVariantMap map = var.toMap();
        RecycleBinItem item;
        item.fileId      = map["fileId"].toString();
        item.fileName    = map["fileName"].toString();
        item.fileIcon    = map["fileIcon"].toString();
        item.isFolder    = map["isFolder"].toBool();
        item.isSelected  = false;
        item.fileSize    = map["fileSize"].toLongLong();
        item.fileSizeStr = map["fileSizeStr"].toString();
        item.deletedAt   = map["deletedAt"].toString();
        m_items.append(item);
    }
    endResetModel();
    emit selectionChanged();
    emit countChanged();
}

void RecycleBinModel::toggleSelection(int row)
{
    if (row < 0 || row >= m_items.count()) return;
    m_items[row].isSelected = !m_items[row].isSelected;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {IsSelectedRole});
    emit selectionChanged();
}

void RecycleBinModel::selectAll()
{
    for (int i = 0; i < m_items.count(); ++i)
        m_items[i].isSelected = true;
    if (!m_items.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

void RecycleBinModel::clearSelection()
{
    for (int i = 0; i < m_items.count(); ++i)
        m_items[i].isSelected = false;
    if (!m_items.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

int RecycleBinModel::selectedCount() const
{
    int count = 0;
    for (const auto &item : m_items)
        if (item.isSelected) count++;
    return count;
}

bool RecycleBinModel::hasSelection() const
{
    return selectedCount() > 0;
}

QVariantMap RecycleBinModel::getFileInfo(int index) const
{
    QVariantMap info;
    if (index < 0 || index >= m_items.count()) return info;
    const RecycleBinItem &item = m_items[index];
    info["fileId"]      = item.fileId;
    info["fileName"]    = item.fileName;
    info["fileIcon"]    = item.fileIcon;
    info["isFolder"]    = item.isFolder;
    info["isSelected"]  = item.isSelected;
    info["fileSize"]    = item.fileSize;
    info["fileSizeStr"] = item.fileSizeStr;
    info["deletedAt"]   = item.deletedAt;
    return info;
}

void RecycleBinModel::removeFileById(const QString &fileId)
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items[i].fileId == fileId) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();
            emit selectionChanged();
            emit countChanged();
            return;
        }
    }
}

// ═══════════════════════════════════════════════
//  RecycleBinController
// ═══════════════════════════════════════════════

RecycleBinController::RecycleBinController(QObject *parent)
    : QObject(parent)
{
    m_model = new RecycleBinModel(this);
    connect(m_model, &RecycleBinModel::selectionChanged, this, &RecycleBinController::selectionStateChanged);
}

RecycleBinController* RecycleBinController::instance()
{
    static RecycleBinController inst;
    return &inst;
}

int  RecycleBinController::selectedCount() const { return m_model->selectedCount(); }
bool RecycleBinController::hasSelection()  const { return m_model->hasSelection(); }

void RecycleBinController::setViewMode(int mode)
{
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    emit viewModeChanged();
}

void RecycleBinController::toggleSelection(int index) { m_model->toggleSelection(index); }
void RecycleBinController::selectAll()                { m_model->selectAll(); }
void RecycleBinController::clearSelection()           { m_model->clearSelection(); }

// ── 加载回收站列表 ──
void RecycleBinController::loadTrash()
{
    qDebug() << "[回收站] 加载回收站列表...";

    QJsonObject params;
    params["skip"]  = 0;
    params["limit"] = 50;

    NetworkManager::instance()->get(
        "/files/trash", params,
        [this](const QJsonObject &data) {
            QJsonArray files = data["files"].toArray();
            QVariantList items;
            for (const QJsonValue &val : files) {
                QJsonObject f = val.toObject();
                QVariantMap item;
                item["fileId"]      = QString::number(f["id"].toInt());
                item["fileName"]    = f["name"].toString();
                item["isFolder"]    = f["is_folder"].toBool();
                item["fileIcon"]    = fileIconForName(f["name"].toString(), f["is_folder"].toBool());
                item["fileSize"]    = f["size"].toVariant().toLongLong();
                item["fileSizeStr"] = f["is_folder"].toBool() ? "-" : formatFileSize(f["size"].toVariant().toLongLong());
                item["deletedAt"]   = f["deleted_at"].toString();
                items.append(item);
            }
            m_model->loadData(items);
            qDebug() << "[回收站] 加载完成, 共" << items.size() << "个文件";
        },
        [](const QString &err) {
            qWarning() << "[回收站] 加载失败:" << err;
        }
    );
}

// ── 放回（还原单个） ──
void RecycleBinController::restoreFile(const QString &fileId)
{
    qDebug() << "[回收站] 还原文件:" << fileId;

    NetworkManager::instance()->post(
        "/files/" + fileId + "/restore", QJsonObject(),
        [this, fileId](const QJsonObject &data) {
            m_model->removeFileById(fileId);

            int affected = data["affected_count"].toInt(1);
            bool renamed = data["was_renamed"].toBool();
            bool toRoot  = data["restored_to_root"].toBool();
            QString newName = data["new_name"].toString();

            QString msg = QString("已恢复 %1 个节点").arg(affected);
            if (renamed) {
                msg += QString("（重命名为「%1」）").arg(newName);
            }
            if (toRoot) {
                msg += "（已恢复到根目录）";
            }

            qDebug() << "[回收站] 还原成功:" << fileId << msg;
            emit trashOpSuccess(msg);
        },
        [fileId](const QString &errMsg) {
            qWarning() << "[回收站] 还原失败:" << fileId << errMsg;
        }
    );
}

void RecycleBinController::restoreSelected()
{
    QStringList ids;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_model->data(idx, RecycleBinModel::IsSelectedRole).toBool()) {
            ids.append(m_model->data(idx, RecycleBinModel::FileIdRole).toString());
        }
    }
    qDebug() << "[回收站] 批量还原:" << ids.size() << "个文件";
    for (const QString &fid : ids) {
        restoreFile(fid);
    }
}

// ── 恢复全部 ──
void RecycleBinController::restoreAll()
{
    qDebug() << "[回收站] 恢复全部文件";

    NetworkManager::instance()->post(
        "/files/trash/restore", QJsonObject(),
        [this](const QJsonObject &data) {
            int totalRestored = data["total_restored_nodes"].toInt();
            int topLevel      = data["top_level_count"].toInt();
            int renamed       = data["conflict_renamed_count"].toInt();
            int fallback      = data["fallback_to_root_count"].toInt();

            // 清空本地 model
            m_model->loadData(QVariantList());

            QString msg = QString("已恢复全部 %1 个项目（共 %2 个节点）").arg(topLevel).arg(totalRestored);
            if (renamed > 0) {
                msg += QString("，%1 个被重命名").arg(renamed);
            }
            if (fallback > 0) {
                msg += QString("，%1 个恢复到根目录").arg(fallback);
            }

            qDebug() << "[回收站] 全部恢复成功:" << msg;
            emit trashOpSuccess(msg);
        },
        [](const QString &errMsg) {
            qWarning() << "[回收站] 全部恢复失败:" << errMsg;
        }
    );
}

// ── 永久删除 ──
void RecycleBinController::permanentDelete(const QString &fileId)
{
    qDebug() << "[回收站] 永久删除:" << fileId;

    QString apiPath = "/files/" + fileId + "/permanent";

    NetworkManager::instance()->deleteResource(
        apiPath,
        [this, fileId](const QJsonObject &data) {
            // 从本地 model 移除
            m_model->removeFileById(fileId);

            int deletedCount = data["deleted_count"].toInt(1);
            qint64 freedSize = data["freed_size"].toVariant().toLongLong();

            qDebug() << "[回收站] 永久删除成功:" << fileId
                     << "deleted:" << deletedCount
                     << "freed:" << freedSize;

            QString msg;
            if (deletedCount <= 1) {
                msg = QString("已永久删除, 释放 %1").arg(formatFileSize(freedSize));
            } else {
                msg = QString("已永久删除 %1 个文件, 释放 %2")
                          .arg(deletedCount)
                          .arg(formatFileSize(freedSize));
            }
            emit trashOpSuccess(msg);
        },
        [fileId](const QString &errMsg) {
            qWarning() << "[回收站] 永久删除失败:" << fileId << errMsg;
        }
    );
}

void RecycleBinController::deleteSelected()
{
    QStringList ids;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_model->data(idx, RecycleBinModel::IsSelectedRole).toBool()) {
            ids.append(m_model->data(idx, RecycleBinModel::FileIdRole).toString());
        }
    }
    qDebug() << "[回收站] 批量永久删除:" << ids.size() << "个文件";
    for (const QString &fid : ids) {
        permanentDelete(fid);
    }
}

// ── 清空回收站 ──
void RecycleBinController::emptyTrash()
{
    qDebug() << "[回收站] 清空回收站";

    NetworkManager::instance()->deleteResource(
        "/files/trash",
        [this](const QJsonObject &data) {
            // 清空本地 model
            m_model->loadData(QVariantList());

            int deletedCount     = data["deleted_count"].toInt();
            int deletedFileCount = data["deleted_file_count"].toInt();
            qint64 freedSize     = data["freed_size"].toVariant().toLongLong();

            qDebug() << "[回收站] 清空成功, deleted:" << deletedCount
                     << "files:" << deletedFileCount
                     << "freed:" << freedSize;

            QString msg = QString("回收站已清空, 删除 %1 个文件, 释放 %2")
                              .arg(deletedFileCount)
                              .arg(formatFileSize(freedSize));
            emit trashOpSuccess(msg);
        },
        [](const QString &errMsg) {
            qWarning() << "[回收站] 清空失败:" << errMsg;
        }
    );
}

// ── 工具方法（复用 FileController 的逻辑） ──
QString RecycleBinController::fileIconForName(const QString &fileName, bool isFolder)
{
    const QString prefix = "qrc:/file_type/res/file_type/";
    if (isFolder) return prefix + "ft-folder.svg";

    QString suffix = QFileInfo(fileName).suffix().toLower();

    if (suffix == "mp4" || suffix == "avi" || suffix == "mkv" || suffix == "mov" ||
        suffix == "wmv" || suffix == "flv" || suffix == "webm" || suffix == "m4v")
        return prefix + "ft-video.svg";
    if (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "aac" ||
        suffix == "ogg" || suffix == "wma" || suffix == "m4a")
        return prefix + "ft-audio.svg";
    if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "gif" ||
        suffix == "bmp" || suffix == "webp" || suffix == "svg" || suffix == "ico" || suffix == "tiff")
        return prefix + "ft-image.svg";
    if (suffix == "pdf")   return prefix + "ft-pdf.svg";
    if (suffix == "doc" || suffix == "docx") return prefix + "ft-word.svg";
    if (suffix == "xls" || suffix == "xlsx") return prefix + "ft-excel.svg";
    if (suffix == "ppt" || suffix == "pptx") return prefix + "ft-ppt.svg";
    if (suffix == "zip" || suffix == "rar" || suffix == "7z" || suffix == "tar" ||
        suffix == "gz"  || suffix == "bz2" || suffix == "xz")
        return prefix + "ft-archive.svg";
    if (suffix == "txt" || suffix == "md"  || suffix == "log" || suffix == "csv" ||
        suffix == "json"|| suffix == "xml" || suffix == "yaml"|| suffix == "yml")
        return prefix + "ft-text.svg";
    if (suffix == "cpp" || suffix == "h"   || suffix == "py"  || suffix == "js"  ||
        suffix == "ts"  || suffix == "java"|| suffix == "c"   || suffix == "cs"  ||
        suffix == "go"  || suffix == "rs"  || suffix == "swift"|| suffix == "kt"  ||
        suffix == "rb"  || suffix == "php" || suffix == "sh"  || suffix == "bat" ||
        suffix == "qml" || suffix == "html"|| suffix == "css")
        return prefix + "ft-code.svg";
    if (suffix == "exe" || suffix == "msi") return prefix + "ft-exe.svg";
    if (suffix == "apk") return prefix + "ft-apk.svg";
    if (suffix == "dmg" || suffix == "pkg") return prefix + "ft-apple-pkg.svg";
    if (suffix == "iso" || suffix == "img") return prefix + "ft-iso.svg";
    if (suffix == "psd" || suffix == "ai")  return prefix + "ft-psd.svg";
    if (suffix == "dwg" || suffix == "dxf") return prefix + "ft-cad.svg";
    if (suffix == "htm" || suffix == "mhtml"|| suffix == "url") return prefix + "ft-web.svg";

    return prefix + "ft-unknown.svg";
}

QString RecycleBinController::formatFileSize(qint64 bytes)
{
    if (bytes < 0) return "-";
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;
    const double TB = GB * 1024;
    if (bytes < KB) return QString::number(bytes) + " B";
    if (bytes < MB) return QString::number(bytes / KB, 'f', 1) + " KB";
    if (bytes < GB) return QString::number(bytes / MB, 'f', 2) + " MB";
    if (bytes < TB) return QString::number(bytes / GB, 'f', 2) + " GB";
    return QString::number(bytes / TB, 'f', 2) + " TB";
}
