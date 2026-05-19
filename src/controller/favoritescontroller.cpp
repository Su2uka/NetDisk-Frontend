#include "favoritescontroller.h"
#include "../network/networkmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDebug>

// ═══════════════════════════════════════════════
//  FavoritesController
// ═══════════════════════════════════════════════

FavoritesController::FavoritesController(QObject *parent)
    : QObject(parent)
{
    m_model = new FileListModel(this);
    connect(m_model, &FileListModel::selectionChanged, this, &FavoritesController::selectionStateChanged);
}

FavoritesController* FavoritesController::instance()
{
    static FavoritesController inst;
    return &inst;
}

int  FavoritesController::selectedCount() const { return m_model->selectedCount(); }
bool FavoritesController::hasSelection()  const { return m_model->hasSelection(); }

void FavoritesController::setViewMode(int mode)
{
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    emit viewModeChanged();
}

void FavoritesController::setSortField(int field)
{
    if (m_sortField == field) return;
    m_sortField = field;
    emit sortFieldChanged();
    loadFavorites();
}

void FavoritesController::setSortAsc(bool asc)
{
    if (m_sortAsc == asc) return;
    m_sortAsc = asc;
    emit sortAscChanged();
    loadFavorites();
}

void FavoritesController::toggleSelection(int index) { m_model->toggleSelection(index); }
void FavoritesController::selectAll()                { m_model->selectAll(); }
void FavoritesController::clearSelection()           { m_model->clearSelection(); }

// ── 加载收藏列表 ──
void FavoritesController::loadFavorites()
{
    qDebug() << "[我的收藏] 加载收藏列表...";
    m_currentPage = 1;
    m_hasMore = false;
    emit hasMoreChanged();

    QJsonObject params;
    params["page"]      = 1;
    params["page_size"] = PAGE_SIZE;

    // 排序参数
    static const char* sortFields[] = {"name", "created_at", "updated_at", "size"};
    if (m_sortField >= 0 && m_sortField <= 3) {
        params["sort_by"] = QString(sortFields[m_sortField]);
    }
    params["order"] = m_sortAsc ? "asc" : "desc";

    NetworkManager::instance()->get(
        "/favorites/list", params,
        [this](const QJsonObject &data) {
            QJsonArray fileArray = data["files"].toArray();
            QVariantList items;

            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();

                QVariantMap item;
                item["fileId"]      = obj["id"].toVariant().toString();
                item["fileName"]    = obj["name"].toString();
                item["isFolder"]    = obj["is_folder"].toBool();
                item["selected"]    = false;
                item["fileIcon"]    = fileIconForName(obj["name"].toString(), obj["is_folder"].toBool());

                item["createTime"] = obj.contains("created_at") ? obj["created_at"].toString() : "";
                item["modifyTime"] = obj.contains("updated_at") ? obj["updated_at"].toString() : "";

                if (obj["is_folder"].toBool()) {
                    item["fileSize"]    = 0;
                    item["fileSizeStr"] = "-";
                } else {
                    qint64 size = obj["size"].toVariant().toLongLong();
                    item["fileSize"]    = size;
                    item["fileSizeStr"] = formatFileSize(size);
                }

                // 保存 parentId 用于跳转到文件位置
                item["parentId"] = obj.contains("parent_id") && !obj["parent_id"].isNull()
                                       ? obj["parent_id"].toVariant().toString()
                                       : "";

                items.append(item);
            }

            m_hasMore = data["has_more"].toBool(false);
            emit hasMoreChanged();

            m_model->loadData(items);
            qDebug() << "[我的收藏] 加载完成, 共" << items.size() << "个收藏";
        },
        [](const QString &err) {
            qWarning() << "[我的收藏] 加载失败:" << err;
        }
    );
}

// ── 加载更多（分页） ──
void FavoritesController::loadMoreFavorites()
{
    if (!m_hasMore || m_loadingMore) return;
    m_loadingMore = true;
    emit loadingMoreChanged();

    m_currentPage++;

    QJsonObject params;
    params["page"]      = m_currentPage;
    params["page_size"] = PAGE_SIZE;

    static const char* sortFields[] = {"name", "created_at", "updated_at", "size"};
    if (m_sortField >= 0 && m_sortField <= 3) {
        params["sort_by"] = QString(sortFields[m_sortField]);
    }
    params["order"] = m_sortAsc ? "asc" : "desc";

    NetworkManager::instance()->get(
        "/favorites/list", params,
        [this](const QJsonObject &data) {
            QJsonArray fileArray = data["files"].toArray();
            QVariantList items;

            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();

                QVariantMap item;
                item["fileId"]      = obj["id"].toVariant().toString();
                item["fileName"]    = obj["name"].toString();
                item["isFolder"]    = obj["is_folder"].toBool();
                item["selected"]    = false;
                item["fileIcon"]    = fileIconForName(obj["name"].toString(), obj["is_folder"].toBool());

                item["createTime"] = obj.contains("created_at") ? obj["created_at"].toString() : "";
                item["modifyTime"] = obj.contains("updated_at") ? obj["updated_at"].toString() : "";

                if (obj["is_folder"].toBool()) {
                    item["fileSize"]    = 0;
                    item["fileSizeStr"] = "-";
                } else {
                    qint64 size = obj["size"].toVariant().toLongLong();
                    item["fileSize"]    = size;
                    item["fileSizeStr"] = formatFileSize(size);
                }

                item["parentId"] = obj.contains("parent_id") && !obj["parent_id"].isNull()
                                       ? obj["parent_id"].toVariant().toString()
                                       : "";

                items.append(item);
            }

            m_hasMore = data["has_more"].toBool(false);
            emit hasMoreChanged();
            m_loadingMore = false;
            emit loadingMoreChanged();
            m_model->appendData(items);
            qDebug() << "[我的收藏] 加载更多完成, 追加" << items.size() << "项";
        },
        [this](const QString &err) {
            qWarning() << "[我的收藏] 加载更多失败:" << err;
            m_loadingMore = false;
            emit loadingMoreChanged();
        }
    );
}

// ── 添加收藏 ──
void FavoritesController::addFavorite(const QString &fileId)
{
    qDebug() << "[我的收藏] 添加收藏:" << fileId;

    QJsonObject body;
    body["file_id"] = fileId.toInt();

    NetworkManager::instance()->post(
        "/favorites/add", body,
        [this](const QJsonObject &) {
            emit favOpSuccess("已添加到收藏");
        },
        [this](const QString &err) {
            qWarning() << "[我的收藏] 添加收藏失败:" << err;
            emit favOpFailed("添加收藏失败：" + err);
        }
    );
}

// ── 取消收藏 ──
void FavoritesController::removeFavorite(const QString &fileId)
{
    qDebug() << "[我的收藏] 取消收藏:" << fileId;

    NetworkManager::instance()->deleteResource(
        "/favorites/" + fileId,
        [this, fileId](const QJsonObject &) {
            m_model->removeFileById(fileId);
            emit favOpSuccess("已取消收藏");
        },
        [this](const QString &err) {
            qWarning() << "[我的收藏] 取消收藏失败:" << err;
            emit favOpFailed("取消收藏失败：" + err);
        }
    );
}

void FavoritesController::removeSelectedFavorites()
{
    QList<QString> ids;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_model->data(idx, FileListModel::IsSelectedRole).toBool()) {
            ids.append(m_model->data(idx, FileListModel::FileIdRole).toString());
        }
    }
    if (ids.isEmpty()) return;

    QJsonArray idsArray;
    for (const QString &id : ids) {
        idsArray.append(id.toInt());
    }

    QJsonObject body;
    body["file_ids"] = idsArray;

    qDebug() << "[我的收藏] 批量取消收藏:" << ids.size() << "个";

    NetworkManager::instance()->post(
        "/favorites/batch-remove", body,
        [this, ids](const QJsonObject &) {
            for (const QString &id : ids) {
                m_model->removeFileById(id);
            }
            emit favOpSuccess(QString("已取消 %1 个收藏").arg(ids.size()));
        },
        [this](const QString &err) {
            qWarning() << "[我的收藏] 批量取消收藏失败:" << err;
            emit favOpFailed("批量取消收藏失败：" + err);
        }
    );
}

// ── 文件图标映射（复用逻辑） ──
QString FavoritesController::fileIconForName(const QString &fileName, bool isFolder)
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

QString FavoritesController::formatFileSize(qint64 bytes)
{
    if (bytes < 0) return "-";
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;
    if (bytes < KB) return QString::number(bytes) + " B";
    if (bytes < MB) return QString::number(bytes / KB, 'f', 1) + " KB";
    if (bytes < GB) return QString::number(bytes / MB, 'f', 2) + " MB";
    return QString::number(bytes / GB, 'f', 2) + " GB";
}
