#include "mysharecontroller.h"
#include "../network/networkmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>

// ═══════════════════════════════════════════════
//  MyShareModel
// ═══════════════════════════════════════════════

MyShareModel::MyShareModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MyShareModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.count();
}

QVariant MyShareModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
        return QVariant();

    const MyShareItem &item = m_items[index.row()];
    switch (role) {
    case ShareIdRole:    return item.shareId;
    case ShareKeyRole:   return item.shareKey;
    case FileIdRole:     return item.fileId;
    case FileNameRole:   return item.fileName;
    case FileIconRole:   return item.fileIcon;
    case IsFolderRole:   return item.isFolder;
    case IsSelectedRole: return item.isSelected;
    case ViewCountRole:  return item.viewCount;
    case SaveCountRole:  return item.saveCount;
    case CreatedAtRole:  return item.createdAt;
    case ExpireAtRole:   return item.expireAt;
    case StatusRole:     return item.status;
    }
    return QVariant();
}

QHash<int, QByteArray> MyShareModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ShareIdRole]    = "shareId";
    roles[ShareKeyRole]   = "shareKey";
    roles[FileIdRole]     = "fileId";
    roles[FileNameRole]   = "fileName";
    roles[FileIconRole]   = "fileIcon";
    roles[IsFolderRole]   = "isFolder";
    roles[IsSelectedRole] = "selected";
    roles[ViewCountRole]  = "viewCount";
    roles[SaveCountRole]  = "saveCount";
    roles[CreatedAtRole]  = "createdAt";
    roles[ExpireAtRole]   = "expireAt";
    roles[StatusRole]     = "status";
    return roles;
}

void MyShareModel::loadData(const QVariantList &data)
{
    beginResetModel();
    m_items.clear();
    for (const QVariant &var : data) {
        QVariantMap map = var.toMap();
        MyShareItem item;
        item.shareId    = map["shareId"].toInt();
        item.shareKey   = map["shareKey"].toString();
        item.fileId     = map["fileId"].toInt();
        item.fileName   = map["fileName"].toString();
        item.fileIcon   = map["fileIcon"].toString();
        item.isFolder   = map["isFolder"].toBool();
        item.isSelected = false;
        item.viewCount  = map["viewCount"].toInt();
        item.saveCount  = map["saveCount"].toInt();
        item.createdAt  = map["createdAt"].toString();
        item.expireAt   = map["expireAt"].toString();
        item.status     = map["status"].toString();
        m_items.append(item);
    }
    endResetModel();
    emit selectionChanged();
    emit countChanged();
}

void MyShareModel::toggleSelection(int row)
{
    if (row < 0 || row >= m_items.count()) return;
    m_items[row].isSelected = !m_items[row].isSelected;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {IsSelectedRole});
    emit selectionChanged();
}

void MyShareModel::selectAll()
{
    for (int i = 0; i < m_items.count(); ++i)
        m_items[i].isSelected = true;
    if (!m_items.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

void MyShareModel::clearSelection()
{
    for (int i = 0; i < m_items.count(); ++i)
        m_items[i].isSelected = false;
    if (!m_items.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_items.count() - 1, 0), {IsSelectedRole});
        emit selectionChanged();
    }
}

int MyShareModel::selectedCount() const
{
    int count = 0;
    for (const auto &item : m_items)
        if (item.isSelected) count++;
    return count;
}

bool MyShareModel::hasSelection() const
{
    return selectedCount() > 0;
}

QVariantMap MyShareModel::getShareInfo(int index) const
{
    QVariantMap info;
    if (index < 0 || index >= m_items.count()) return info;
    const MyShareItem &item = m_items[index];
    info["shareId"]    = item.shareId;
    info["shareKey"]   = item.shareKey;
    info["fileId"]     = item.fileId;
    info["fileName"]   = item.fileName;
    info["fileIcon"]   = item.fileIcon;
    info["isFolder"]   = item.isFolder;
    info["isSelected"] = item.isSelected;
    info["viewCount"]  = item.viewCount;
    info["saveCount"]  = item.saveCount;
    info["createdAt"]  = item.createdAt;
    info["expireAt"]   = item.expireAt;
    info["status"]     = item.status;
    return info;
}

void MyShareModel::removeShareById(int shareId)
{
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items[i].shareId == shareId) {
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
//  MyShareController
// ═══════════════════════════════════════════════

MyShareController::MyShareController(QObject *parent)
    : QObject(parent)
{
    m_model = new MyShareModel(this);
    connect(m_model, &MyShareModel::selectionChanged, this, &MyShareController::selectionStateChanged);
}

MyShareController* MyShareController::instance()
{
    static MyShareController inst;
    return &inst;
}

int  MyShareController::selectedCount() const { return m_model->selectedCount(); }
bool MyShareController::hasSelection()  const { return m_model->hasSelection(); }

void MyShareController::setSortField(int field)
{
    if (m_sortField == field) return;
    m_sortField = field;
    emit sortFieldChanged();
    loadShares();
}

void MyShareController::setSortAsc(bool asc)
{
    if (m_sortAsc == asc) return;
    m_sortAsc = asc;
    emit sortAscChanged();
    loadShares();
}

void MyShareController::toggleSelection(int index) { m_model->toggleSelection(index); }
void MyShareController::selectAll()                { m_model->selectAll(); }
void MyShareController::clearSelection()           { m_model->clearSelection(); }

// ── 排序字段映射 ──
QString MyShareController::sortFieldToString(int field)
{
    switch (field) {
    case 0:  return "created_at";
    case 1:  return "name";
    case 2:  return "view_count";
    case 3:  return "save_count";
    default: return "created_at";
    }
}

// ── 加载分享列表 ──
void MyShareController::loadShares()
{
    qDebug() << "[我的分享] 加载分享列表...";
    m_currentPage = 1;

    QJsonObject params;
    params["page"]      = m_currentPage;
    params["page_size"]  = 50;
    params["sort_by"]   = sortFieldToString(m_sortField);
    params["order"]     = m_sortAsc ? "asc" : "desc";

    NetworkManager::instance()->get(
        "/shares/list", params,
        [this](const QJsonObject &data) {
            QJsonArray shares = data["shares"].toArray();
            QVariantList items;
            for (const QJsonValue &val : shares) {
                QJsonObject s = val.toObject();
                QVariantMap item;
                item["shareId"]    = s["id"].toInt();
                item["shareKey"]   = s["share_key"].toString();
                item["fileId"]     = s["file_id"].toInt();
                item["fileName"]   = s["name"].toString();
                item["isFolder"]   = s["is_folder"].toBool();
                item["fileIcon"]   = fileIconForName(s["name"].toString(), s["is_folder"].toBool());
                item["viewCount"]  = s["view_count"].toInt();
                item["saveCount"]  = s["save_count"].toInt();
                item["createdAt"]  = s["created_at"].toString();
                item["expireAt"]   = s["expire_at"].isNull() ? "" : s["expire_at"].toString();
                item["status"]     = s["status"].toString();
                items.append(item);
            }
            m_model->loadData(items);
            m_hasMore = data["has_more"].toBool();
            emit hasMoreChanged();
            qDebug() << "[我的分享] 加载完成, 共" << items.size() << "个分享";
        },
        [](const QString &err) {
            qWarning() << "[我的分享] 加载失败:" << err;
        }
    );
}

// ── 加载更多（分页） ──
void MyShareController::loadMoreShares()
{
    if (!m_hasMore || m_loadingMore) return;
    m_loadingMore = true;
    emit loadingMoreChanged();

    m_currentPage++;

    QJsonObject params;
    params["page"]      = m_currentPage;
    params["page_size"]  = 50;
    params["sort_by"]   = sortFieldToString(m_sortField);
    params["order"]     = m_sortAsc ? "asc" : "desc";

    NetworkManager::instance()->get(
        "/shares/list", params,
        [this](const QJsonObject &data) {
            QJsonArray shares = data["shares"].toArray();
            // 追加到现有模型
            QVariantList items;
            for (const QJsonValue &val : shares) {
                QJsonObject s = val.toObject();
                QVariantMap item;
                item["shareId"]    = s["id"].toInt();
                item["shareKey"]   = s["share_key"].toString();
                item["fileId"]     = s["file_id"].toInt();
                item["fileName"]   = s["name"].toString();
                item["isFolder"]   = s["is_folder"].toBool();
                item["fileIcon"]   = fileIconForName(s["name"].toString(), s["is_folder"].toBool());
                item["viewCount"]  = s["view_count"].toInt();
                item["saveCount"]  = s["save_count"].toInt();
                item["createdAt"]  = s["created_at"].toString();
                item["expireAt"]   = s["expire_at"].isNull() ? "" : s["expire_at"].toString();
                item["status"]     = s["status"].toString();
                items.append(item);
            }
            // TODO: appendData — 暂时重新加载全部
            m_hasMore = data["has_more"].toBool();
            emit hasMoreChanged();
            m_loadingMore = false;
            emit loadingMoreChanged();
        },
        [this](const QString &err) {
            qWarning() << "[我的分享] 加载更多失败:" << err;
            m_loadingMore = false;
            emit loadingMoreChanged();
        }
    );
}

// ── 取消分享 ──
void MyShareController::cancelShare(int shareId)
{
    qDebug() << "[我的分享] 取消分享:" << shareId;

    NetworkManager::instance()->post(
        "/shares/" + QString::number(shareId) + "/cancel", QJsonObject{},
        [this, shareId](const QJsonObject &) {
            m_model->removeShareById(shareId);
            emit shareOpSuccess("已取消分享");
        },
        [this](const QString &err) {
            qWarning() << "[我的分享] 取消分享失败:" << err;
            emit shareOpFailed("取消分享失败：" + err);
        }
    );
}

void MyShareController::cancelSelected()
{
    QList<int> ids;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_model->data(idx, MyShareModel::IsSelectedRole).toBool()) {
            ids.append(m_model->data(idx, MyShareModel::ShareIdRole).toInt());
        }
    }
    if (ids.isEmpty()) return;

    QJsonArray idsArray;
    for (int id : ids) {
        idsArray.append(id);
    }

    QJsonObject body;
    body["share_ids"] = idsArray;

    qDebug() << "[我的分享] 批量取消分享:" << ids.size() << "个";

    NetworkManager::instance()->post(
        "/shares/batch-cancel", body,
        [this, ids](const QJsonObject &) {
            for (int id : ids) {
                m_model->removeShareById(id);
            }
            emit shareOpSuccess(QString("已取消 %1 个分享").arg(ids.size()));
        },
        [this](const QString &err) {
            qWarning() << "[我的分享] 批量取消分享失败:" << err;
            emit shareOpFailed("批量取消分享失败：" + err);
        }
    );
}

// ── 获取分享口令文本 ──
void MyShareController::getShareCode(int index)
{
    QVariantMap info = m_model->getShareInfo(index);
    if (info.isEmpty()) return;

    int shareId = info["shareId"].toInt();
    QString fileName = info["fileName"].toString();

    QString apiPath = "/shares/" + QString::number(shareId) + "/credentials";

    NetworkManager::instance()->get(
        apiPath, QJsonObject{},
        [this, fileName](const QJsonObject &data) {
            QString shareKey    = data["share_key"].toString();
            QString extractCode = data["extract_code"].toString();

            QString text;
            if (extractCode.isEmpty()) {
                text = QString("【轻云盘】我分享了「%1」给你\n口令：%2")
                           .arg(fileName, shareKey);
            } else {
                text = QString("【轻云盘】我分享了「%1」给你\n口令：%2，提取码：%3")
                           .arg(fileName, shareKey, extractCode);
            }

            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(text);

            qDebug() << "[我的分享] 口令已复制:" << text;
            emit shareOpSuccess("分享口令已复制到剪贴板");
        },
        [this](const QString &err) {
            qWarning() << "[我的分享] 获取分享口令失败:" << err;
            emit shareOpFailed("获取分享口令失败：" + err);
        }
    );
}

// ── 文件图标映射（复用逻辑） ──
QString MyShareController::fileIconForName(const QString &fileName, bool isFolder)
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

    return prefix + "ft-unknown.svg";
}
