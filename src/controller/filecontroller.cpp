#include "filecontroller.h"
#include "uploadcontroller.h"
#include "downloadcontroller.h"
#include "../network/networkmanager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

FileController::FileController(QObject *parent)
    : QObject{parent}, m_viewMode(0), m_sortField(1), m_sortAsc(false), m_loading(false)
{
    m_fileModel = new FileListModel(this);

    // 初始化面包屑导航：默认只有一项"全部文件"，其 folderId 为空字符串
    QVariantMap rootNode;
    rootNode["folderId"] = "";
    rootNode["title"] = "全部文件";
    m_breadcrumbs.append(rootNode);

    connect(m_fileModel, &FileListModel::selectionChanged, this, &FileController::selectionStateChanged);
}

// ── 属性获取与设置 ──

FileListModel* FileController::fileModel() const
{
    return m_fileModel;
}

int FileController::viewMode() const
{
    return m_viewMode;
}

void FileController::setViewMode(int mode)
{
    if (m_viewMode != mode) {
        m_viewMode = mode;
        emit viewModeChanged();
    }
}

QVariantList FileController::breadcrumbs() const
{
    return m_breadcrumbs;
}

QString FileController::currentFolderId() const
{
    if (m_breadcrumbs.isEmpty()) {
        return "";
    }
    QVariantMap currentFolder = m_breadcrumbs.last().toMap();
    return currentFolder["folderId"].toString();
}

int FileController::selectedCount() const
{
    return m_fileModel->selectedCount();
}

bool FileController::hasSelection() const
{
    return m_fileModel->hasSelection();
}

int FileController::sortField() const
{
    return m_sortField;
}

void FileController::setSortField(int field)
{
    if (m_sortField != field) {
        m_sortField = field;
        emit sortFieldChanged();
        loadFiles();  // 后端排序，重新加载
    }
}

bool FileController::sortAsc() const
{
    return m_sortAsc;
}

void FileController::setSortAsc(bool asc)
{
    if (m_sortAsc != asc) {
        m_sortAsc = asc;
        emit sortAscChanged();
        loadFiles();  // 后端排序，重新加载
    }
}

bool FileController::loading() const
{
    return m_loading;
}

bool FileController::hasMore() const
{
    return m_hasMore;
}

bool FileController::loadingMore() const
{
    return m_loadingMore;
}

// ── 供 QML 调用的业务接口 ──

void FileController::enterFolder(const QString &folderId, const QString &title)
{
    QVariantMap node;
    node["folderId"] = folderId;
    node["title"] = title;

    m_breadcrumbs.append(node);
    emit breadcrumbsChanged();

    qDebug() << "进入文件夹:" << title << "ID:" << folderId;
    loadFiles();
}

void FileController::goBack()
{
    if (m_breadcrumbs.size() <= 1)
        return;

    m_breadcrumbs.pop_back();
    emit breadcrumbsChanged();

    if (!m_breadcrumbs.isEmpty()) {
        QVariantMap currentFolder = m_breadcrumbs.last().toMap();
        qDebug() << "返回到:" << currentFolder["title"].toString();
        loadFiles();
    }
}

void FileController::navigateTo(int index)
{
    if (index < 0 || index >= m_breadcrumbs.size() - 1)
        return;

    m_breadcrumbs = m_breadcrumbs.mid(0, index + 1);
    emit breadcrumbsChanged();

    qDebug() << "导航到:" << m_breadcrumbs.last().toMap()["title"].toString();
    loadFiles();
}

void FileController::toggleSelection(int index)
{
    m_fileModel->toggleSelection(index);
}

void FileController::selectAll()
{
    m_fileModel->selectAll();
}

void FileController::clearSelection()
{
    m_fileModel->clearSelection();
}

// ════════════════════════════════════════════
//  从后端加载文件列表
// ════════════════════════════════════════════

void FileController::loadFiles()
{
    // 如果有「前往文件位置」的待导航面包屑，直接使用
    if (!m_pendingBreadcrumbs.isEmpty()) {
        m_breadcrumbs = m_pendingBreadcrumbs;
        m_pendingBreadcrumbs.clear();
        emit breadcrumbsChanged();
        qDebug() << "[文件列表] 消费 pendingBreadcrumbs, 层数:" << m_breadcrumbs.size();
    }

    // 保底：面包屑被清空时（如从智能目录返回），重新初始化根节点
    if (m_breadcrumbs.isEmpty()) {
        QVariantMap rootNode;
        rootNode["folderId"] = "";
        rootNode["title"] = "全部文件";
        m_breadcrumbs.append(rootNode);
        emit breadcrumbsChanged();
    }

    // 重置分页状态（首次加载 / 刷新 / 切换目录）
    m_currentPage = 1;
    m_hasMore = false;
    emit hasMoreChanged();

    QString folderId = currentFolderId();

    m_loading = true;
    emit loadingChanged();

    qDebug() << "[文件列表] 请求加载，parent_id:" << (folderId.isEmpty() ? "null (根目录)" : folderId);

    // 构造请求参数
    QJsonObject params;
    if (!folderId.isEmpty()) {
        params["parent_id"] = folderId;
    }

    // 排序参数
    static const char* sortFields[] = {"name", "created_at", "updated_at", "size"};
    if (m_sortField >= 0 && m_sortField <= 3) {
        params["sort_by"] = QString(sortFields[m_sortField]);
    }
    params["order"] = m_sortAsc ? "asc" : "desc";

    // 分页参数
    params["page"] = 1;
    params["page_size"] = PAGE_SIZE;

    NetworkManager::instance()->get(
        "/files/list", params,
        // 成功回调
        [this](const QJsonObject &data) {
            m_loading = false;
            emit loadingChanged();

            // 解析分页元数据
            m_hasMore = data["has_more"].toBool(false);
            emit hasMoreChanged();

            // data
            QJsonArray fileArray;
            fileArray = data["files"].toArray();

            QVariantList items;

            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();

                QVariantMap item;
                item["fileId"]      = obj["id"].toVariant().toString();
                item["fileName"]    = obj["name"].toString();
                item["isFolder"]    = obj["is_folder"].toBool();
                item["selected"]    = false;

                // 图标映射
                item["fileIcon"] = fileIconForName(obj["name"].toString(), obj["is_folder"].toBool());

                // 时间
                item["createTime"] = obj.contains("created_at") ? obj["created_at"].toString() : "";
                item["modifyTime"] = obj.contains("updated_at") ? obj["updated_at"].toString() : "";

                // 文件大小
                if (obj["is_folder"].toBool()) {
                    item["fileSize"] = 0;
                    item["fileSizeStr"] = "-";
                } else {
                    qint64 size = obj["size"].toVariant().toLongLong();
                    item["fileSize"] = size;
                    item["fileSizeStr"] = formatFileSize(size);
                }

                items.append(item);
            }

            qDebug() << "[文件列表] 加载完成，共" << items.size() << "项，hasMore:" << m_hasMore;
            m_fileModel->loadData(items);
        },
        // 失败回调
        [this](const QString &errMsg) {
            m_loading = false;
            emit loadingChanged();
            qWarning() << "[文件列表] 加载失败:" << errMsg;
        }
        );
}

void FileController::loadMoreFiles()
{
    // 防重复请求：正在加载 或 没有更多数据时直接返回
    if (m_loadingMore || !m_hasMore)
        return;

    m_loadingMore = true;
    emit loadingMoreChanged();

    int nextPage = m_currentPage + 1;
    QString folderId = currentFolderId();

    qDebug() << "[文件列表] 加载更多，page:" << nextPage;

    // 构造请求参数
    QJsonObject params;
    if (!folderId.isEmpty()) {
        params["parent_id"] = folderId;
    }

    // 排序参数
    static const char* sortFields[] = {"name", "created_at", "updated_at", "size"};
    if (m_sortField >= 0 && m_sortField <= 3) {
        params["sort_by"] = QString(sortFields[m_sortField]);
    }
    params["order"] = m_sortAsc ? "asc" : "desc";

    // 分页参数
    params["page"] = nextPage;
    params["page_size"] = PAGE_SIZE;

    NetworkManager::instance()->get(
        "/files/list", params,
        [this, nextPage](const QJsonObject &data) {
            m_loadingMore = false;
            emit loadingMoreChanged();

            m_currentPage = nextPage;

            // 更新分页状态
            m_hasMore = data["has_more"].toBool(false);
            emit hasMoreChanged();

            QJsonArray fileArray = data["files"].toArray();
            QVariantList items;

            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();

                QVariantMap item;
                item["fileId"]      = obj["id"].toVariant().toString();
                item["fileName"]    = obj["name"].toString();
                item["isFolder"]    = obj["is_folder"].toBool();
                item["selected"]    = false;

                item["fileIcon"] = fileIconForName(obj["name"].toString(), obj["is_folder"].toBool());

                item["createTime"] = obj.contains("created_at") ? obj["created_at"].toString() : "";
                item["modifyTime"] = obj.contains("updated_at") ? obj["updated_at"].toString() : "";

                if (obj["is_folder"].toBool()) {
                    item["fileSize"] = 0;
                    item["fileSizeStr"] = "-";
                } else {
                    qint64 size = obj["size"].toVariant().toLongLong();
                    item["fileSize"] = size;
                    item["fileSizeStr"] = formatFileSize(size);
                }

                items.append(item);
            }

            qDebug() << "[文件列表] 加载更多完成，追加" << items.size() << "项，hasMore:" << m_hasMore;
            m_fileModel->appendData(items);  // 增量追加，不重置列表
        },
        [this](const QString &errMsg) {
            m_loadingMore = false;
            emit loadingMoreChanged();
            qWarning() << "[文件列表] 加载更多失败:" << errMsg;
        }
    );
}

void FileController::loadFilesByCategory(const QString &category)
{
    m_loading = true;
    emit loadingChanged();

    // 清空
    m_breadcrumbs.clear();
    emit breadcrumbsChanged();
    m_categoryCache.clear();
    m_fileModel->loadData({});
    emit selectionStateChanged();

    qDebug() << "[智能目录] 请求分类:" << category;

    QJsonObject params;
    params["category"] = category;

    NetworkManager::instance()->get(
        "/files/by-category", params,
        [this](const QJsonObject &data) {
            m_loading = false;
            emit loadingChanged();

            QJsonArray fileArray = data["files"].toArray();
            QVariantList items;

            for (const QJsonValue &val : fileArray) {
                QJsonObject obj = val.toObject();

                QVariantMap item;
                item["fileId"]      = obj["id"].toVariant().toString();
                item["fileName"]    = obj["name"].toString();
                item["isFolder"]    = false;
                item["selected"]    = false;

                item["fileIcon"] = fileIconForName(obj["name"].toString(), false);

                // 记录父目录 ID 和名称（用于「前往文件位置」功能）
                item["parentId"] = obj.contains("parent_id") && !obj["parent_id"].isNull()
                                        ? obj["parent_id"].toVariant().toString()
                                        : "";

                item["createTime"] = obj.contains("created_at") ? obj["created_at"].toString() : "";
                item["modifyTime"] = obj.contains("updated_at") ? obj["updated_at"].toString() : "";

                qint64 size = obj["size"].toVariant().toLongLong();
                item["fileSize"]    = size;
                item["fileSizeStr"] = formatFileSize(size);

                items.append(item);
            }

            // 缓存全量数据
            m_categoryCache = items;

            qDebug() << "[智能目录] 加载完成，共" << items.size() << "项（已缓存）";
            m_fileModel->loadData(items);
        },
        [this](const QString &errMsg) {
            m_loading = false;
            emit loadingChanged();
            qWarning() << "[智能目录] 加载失败:" << errMsg;
        }
    );
}

void FileController::filterCategoryByExt(const QString &ext)
{
    // ext 为空 → 显示全部（从缓存恢复）
    if (ext.isEmpty()) {
        m_fileModel->loadData(m_categoryCache);
        emit selectionStateChanged();
        return;
    }

    QVariantList filtered;
    for (const QVariant &v : m_categoryCache) {
        QVariantMap item = v.toMap();
        QString fileName = item["fileName"].toString();
        QString fileExt  = fileName.mid(fileName.lastIndexOf('.') + 1).toLower();

        if (ext == "other") {
            // "其他"：不匹配任何已知子分类 — 由 QML 端判断
            // 这里简单地传 "other"，后续可扩展
            // 暂时跳过 other 的过滤逻辑（显示全部）
            filtered.append(item);
        } else if (fileExt == ext.toLower()) {
            filtered.append(item);
        }
    }

    qDebug() << "[智能目录] 客户端筛选 ext:" << ext << "结果:" << filtered.size() << "项";
    m_fileModel->loadData(filtered);
    emit selectionStateChanged();
}

void FileController::navigateToFileLocation(const QString &parentId)
{
    qDebug() << "[智能目录] 前往文件位置, parentId:" << parentId;

    if (parentId.isEmpty()) {
        // 根目录，无需查询路径，只放“全部文件”
        QVariantList root;
        QVariantMap rootNode;
        rootNode["folderId"] = "";
        rootNode["title"] = "全部文件";
        root.append(rootNode);
        m_pendingBreadcrumbs = root;
        emit goToFileLocationRequested(parentId);
        return;
    }

    // 调用 GET /files/{id}/path 获取完整祖先链
    NetworkManager::instance()->get(
        "/files/" + parentId + "/path", QJsonObject(),
        [this, parentId](const QJsonObject &data) {
            // 固定株底：全部文件
            QVariantList breadcrumbs;
            QVariantMap rootNode;
            rootNode["folderId"] = "";
            rootNode["title"] = "全部文件";
            breadcrumbs.append(rootNode);

            // 解析路径数组: [{"id": 100, "name": "test"}, {"id": 547, "name": "父目录"}]
            QJsonArray pathArray = data["path"].toArray();
            for (const QJsonValue &val : pathArray) {
                QJsonObject node = val.toObject();
                QVariantMap crumb;
                crumb["folderId"] = node["id"].toVariant().toString();
                crumb["title"] = node["name"].toString();
                breadcrumbs.append(crumb);
            }

            qDebug() << "[智能目录] 获取路径成功, 层数:" << breadcrumbs.size();

            m_pendingBreadcrumbs = breadcrumbs;
            emit goToFileLocationRequested(parentId);
        },
        [this, parentId](const QString &errMsg) {
            qWarning() << "[智能目录] 获取路径失败:" << errMsg << "，回退到简单模式";

            // 失败时回退：全部文件 + 一层
            QVariantList fallback;
            QVariantMap rootNode;
            rootNode["folderId"] = "";
            rootNode["title"] = "全部文件";
            fallback.append(rootNode);

            QVariantMap crumb;
            crumb["folderId"] = parentId;
            crumb["title"] = "...";
            fallback.append(crumb);

            m_pendingBreadcrumbs = fallback;
            emit goToFileLocationRequested(parentId);
        }
    );
}

void FileController::createFolder(const QString &folderName)
{
    if (folderName.trimmed().isEmpty()) {
        qWarning() << "[新建文件夹] 文件夹名不能为空";
        return;
    }

    QJsonObject params;
    params["name"] = folderName.trimmed();
    QString parentId = currentFolderId();
    if (!parentId.isEmpty()) {
        params["parent_id"] = parentId.toInt();
    }

    qDebug() << "[新建文件夹] 创建:" << folderName << "parent:" << (parentId.isEmpty() ? "根目录" : parentId);

    NetworkManager::instance()->post(
        "/files/folder", params,
        [this, folderName](const QJsonObject &) {
            qDebug() << "[新建文件夹] 创建成功:" << folderName;
            emit folderCreated("文件夹「" + folderName + "」创建成功");
            loadFiles();  // 刷新当前目录
        },
        [this](const QString &errMsg) {
            qWarning() << "[新建文件夹] 创建失败:" << errMsg;
            emit folderCreated("创建失败：" + errMsg);
        }
    );
}

// ── 上传操作：只负责「入队」，不做任何 I/O ──

void FileController::uploadFiles(const QList<QUrl> &fileUrls)
{
    QString targetFolderId = currentFolderId();

    qDebug() << "收到" << fileUrls.size() << "个文件上传请求，委派给 UploadController 队列";
    qDebug() << "目标目录 ID:" << targetFolderId;

    UploadController::instance()->enqueueFiles(fileUrls, targetFolderId);
}

void FileController::uploadFolder(const QUrl &folderUrl)
{
    QString targetFolderId = currentFolderId();

    qDebug() << "收到文件夹上传请求，委派给 UploadController 队列";
    qDebug() << "目标目录 ID:" << targetFolderId;

    UploadController::instance()->enqueueFolder(folderUrl, targetFolderId);
}

// ════════════════════════════════════════════
//  下载操作：向后端换取预签名 URL，然后委派给 DownloadController
// ════════════════════════════════════════════

void FileController::requestDownload(const QString &fileId,
                                      const QString &fileName,
                                      qint64 fileSize,
                                      const QString &saveDirPath)
{
    qDebug() << "[下载] 请求预签名 URL, fileId:" << fileId << "文件名:" << fileName;

    // 调用后端接口获取预签名下载直链
    QString apiPath = "/files/" + fileId + "/download_url";

    NetworkManager::instance()->get(
        apiPath, QJsonObject(),
        // 成功回调
        [fileId, fileName, fileSize, saveDirPath](const QJsonObject &data) {
            QString preSignedUrl = data["download_url"].toString();

            if (preSignedUrl.isEmpty()) {
                qWarning() << "[下载] 后端返回的预签名 URL 为空";
                return;
            }

            // 拼接本地保存完整路径
            QString localSavePath = saveDirPath + "/" + fileName;

            qDebug() << "[下载] 获取直链成功，入队下载:" << localSavePath;
            DownloadController::instance()->enqueueFile(fileId, fileName, preSignedUrl, localSavePath, fileSize);
        },
        // 失败回调
        [fileName](const QString &errMsg) {
            qWarning() << "[下载] 获取预签名 URL 失败:" << fileName << errMsg;
        }
    );
}

// ════════════════════════════════════════════
//  文件夹下载：获取文件树 → 批量请求预签名 → 入队
// ════════════════════════════════════════════

void FileController::downloadFolder(const QString &folderId,
                                     const QString &folderName,
                                     const QString &saveDirPath)
{
    qDebug() << "[文件夹下载] 请求文件树, folderId:" << folderId
             << "文件夹名:" << folderName
             << "保存路径:" << saveDirPath;

    if (saveDirPath.isEmpty()) {
        qWarning() << "[文件夹下载] 保存路径为空，取消下载";
        return;
    }

    // ── 1. 请求后端的文件夹树状图纸 ──
    QString apiPath = "/files/" + folderId + "/download_tree";

    NetworkManager::instance()->get(
        apiPath, QJsonObject(),
        // 成功回调
        [this, folderName, saveDirPath](const QJsonObject &data) {
            // 解析根文件夹名称和文件列表
            QString rootFolderName = data["root_folder_name"].toString();
            if (rootFolderName.isEmpty()) {
                rootFolderName = folderName;  // 兜底使用传入的文件夹名
            }

            QJsonArray files = data["files"].toArray();
            if (files.isEmpty()) {
                qWarning() << "[文件夹下载] 该文件夹下没有可下载的文件";
                return;
            }

            qDebug() << "[文件夹下载] 文件树获取成功, 根目录:" << rootFolderName
                     << "文件数量:" << files.count();

            // ── 2. 拼接本地根路径 ──
            QString localRootPath = saveDirPath + "/" + rootFolderName;
            qDebug() << "[文件夹下载] 本地根路径:" << localRootPath;

            // ── 3. 递归串行获取每个文件的预签名 URL，然后批量入队 ──
            QVariantList *results = new QVariantList();
            fetchDownloadUrlsAndEnqueue(files, localRootPath, 0, results);
        },
        // 失败回调
        [folderName](const QString &errMsg) {
            qWarning() << "[文件夹下载] 获取文件树失败:" << folderName << errMsg;
        }
    );
}

void FileController::fetchDownloadUrlsAndEnqueue(const QJsonArray &files,
                                                   const QString &localRootPath,
                                                   int index,
                                                   QVariantList *accumulatedResults)
{
    // ── 递归终止：所有文件的预签名 URL 都已收集完毕 ──
    if (index >= files.count()) {
        qDebug() << "[文件夹下载] 所有预签名 URL 获取完毕, 共"
                 << accumulatedResults->count() << "个文件，开始入队";

        // 调用 DownloadController 批量入队（内部已含 mkpath + 批量创建任务）
        DownloadController::instance()->enqueueFolder(*accumulatedResults, localRootPath);

        // 释放堆上的临时容器
        delete accumulatedResults;
        return;
    }

    // ── 取出当前文件的信息 ──
    QJsonObject fileObj = files[index].toObject();
    int fileId          = fileObj["file_id"].toInt();
    QString relPath     = fileObj["relative_path"].toString();   // 如 "A/B/c.txt"
    qint64 fileSize     = fileObj["size"].toVariant().toLongLong();

    // 从 relative_path 中提取文件名和相对目录
    // 例如 "A/B/c.txt" → fileName = "c.txt", relativeDir = "A/B"
    int lastSlash = relPath.lastIndexOf('/');
    QString fileName    = (lastSlash >= 0) ? relPath.mid(lastSlash + 1) : relPath;
    QString relativeDir = (lastSlash >= 0) ? relPath.left(lastSlash) : QString();

    // ── 请求该文件的预签名下载 URL ──
    QString apiPath = "/files/" + QString::number(fileId) + "/download_url";

    NetworkManager::instance()->get(
        apiPath, QJsonObject(),
        // 成功回调：收集结果，然后递归处理下一个文件
        [this, files, localRootPath, index, accumulatedResults,
         fileId, fileName, relativeDir, fileSize](const QJsonObject &data) {

            QString preSignedUrl = data["download_url"].toString();
            if (preSignedUrl.isEmpty()) {
                qWarning() << "[文件夹下载] 文件" << fileName << "的预签名 URL 为空，跳过";
            } else {
                // 构建 enqueueFolder 需要的 QVariantMap
                QVariantMap fileEntry;
                fileEntry["fileId"]       = fileId;
                fileEntry["relativePath"] = relativeDir;   // 相对目录（不含文件名）
                fileEntry["fileName"]     = fileName;
                fileEntry["preSignedUrl"] = preSignedUrl;
                fileEntry["fileSize"]     = fileSize;

                accumulatedResults->append(fileEntry);
            }

            // 递归：处理下一个文件
            fetchDownloadUrlsAndEnqueue(files, localRootPath, index + 1, accumulatedResults);
        },
        // 失败回调：跳过当前文件，继续处理下一个
        [this, files, localRootPath, index, accumulatedResults, fileName](const QString &errMsg) {
            qWarning() << "[文件夹下载] 获取预签名 URL 失败:" << fileName << errMsg;

            // 不中断整个流程，继续下一个文件
            fetchDownloadUrlsAndEnqueue(files, localRootPath, index + 1, accumulatedResults);
        }
    );
}

// ════════════════════════════════════════════
//  辅助工具函数
// ════════════════════════════════════════════

QString FileController::fileIconForName(const QString &fileName, bool isFolder)
{
    const QString prefix = "qrc:/file_type/res/file_type/";

    if (isFolder)
        return prefix + "ft-folder.svg";

    // 取文件后缀（小写）
    QString suffix = QFileInfo(fileName).suffix().toLower();

    // 视频
    if (suffix == "mp4" || suffix == "avi" || suffix == "mkv" || suffix == "mov" ||
        suffix == "wmv" || suffix == "flv" || suffix == "webm" || suffix == "m4v")
        return prefix + "ft-video.svg";

    // 音频
    if (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "aac" ||
        suffix == "ogg" || suffix == "wma" || suffix == "m4a")
        return prefix + "ft-audio.svg";

    // 图片
    if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "gif" ||
        suffix == "bmp" || suffix == "webp" || suffix == "svg" || suffix == "ico" || suffix == "tiff")
        return prefix + "ft-image.svg";

    // 文档
    if (suffix == "pdf")
        return prefix + "ft-pdf.svg";
    if (suffix == "doc" || suffix == "docx")
        return prefix + "ft-word.svg";
    if (suffix == "xls" || suffix == "xlsx")
        return prefix + "ft-excel.svg";
    if (suffix == "ppt" || suffix == "pptx")
        return prefix + "ft-ppt.svg";

    // 压缩包
    if (suffix == "zip" || suffix == "rar" || suffix == "7z" || suffix == "tar" ||
        suffix == "gz" || suffix == "bz2" || suffix == "xz")
        return prefix + "ft-archive.svg";

    // 文本
    if (suffix == "txt" || suffix == "md" || suffix == "log" || suffix == "csv" ||
        suffix == "json" || suffix == "xml" || suffix == "yaml" || suffix == "yml")
        return prefix + "ft-text.svg";

    // 代码
    if (suffix == "cpp" || suffix == "h" || suffix == "py" || suffix == "js" ||
        suffix == "ts" || suffix == "java" || suffix == "c" || suffix == "cs" ||
        suffix == "go" || suffix == "rs" || suffix == "swift" || suffix == "kt" ||
        suffix == "rb" || suffix == "php" || suffix == "sh" || suffix == "bat" ||
        suffix == "qml" || suffix == "html" || suffix == "css")
        return prefix + "ft-code.svg";

    // 可执行文件
    if (suffix == "exe" || suffix == "msi")
        return prefix + "ft-exe.svg";

    // 安装包
    if (suffix == "apk")
        return prefix + "ft-apk.svg";
    if (suffix == "dmg" || suffix == "pkg")
        return prefix + "ft-apple-pkg.svg";

    // 镜像
    if (suffix == "iso" || suffix == "img")
        return prefix + "ft-iso.svg";

    // PSD
    if (suffix == "psd" || suffix == "ai")
        return prefix + "ft-psd.svg";

    // CAD
    if (suffix == "dwg" || suffix == "dxf")
        return prefix + "ft-cad.svg";

    // 网页
    if (suffix == "htm" || suffix == "mhtml" || suffix == "url")
        return prefix + "ft-web.svg";

    // 兜底
    return prefix + "ft-unknown.svg";
}

QString FileController::formatFileSize(qint64 bytes)
{
    if (bytes < 0)
        return "-";

    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;
    const double TB = GB * 1024;

    if (bytes < KB)
        return QString::number(bytes) + " B";
    if (bytes < MB)
        return QString::number(bytes / KB, 'f', 1) + " KB";
    if (bytes < GB)
        return QString::number(bytes / MB, 'f', 2) + " MB";
    if (bytes < TB)
        return QString::number(bytes / GB, 'f', 2) + " GB";
    return QString::number(bytes / TB, 'f', 2) + " TB";
}

// ── 回收站 ──

void FileController::moveToTrash(const QString &fileId)
{
    qDebug() << "[回收站] 移入回收站, fileId:" << fileId;

    QString apiPath = "/files/" + fileId + "/trash";

    NetworkManager::instance()->deleteResource(
        apiPath,
        // 成功回调：解析 data 并操作本地 model
        [this, fileId](const QJsonObject &data) {
            // 1. 无论 already_in_trash 是否为 true，都从本地 model 移除
            m_fileModel->removeFileById(fileId);

            // 2. 提取字段
            bool alreadyInTrash = data["already_in_trash"].toBool();
            int affectedCount   = data["affected_count"].toInt(1);

            qDebug() << "[回收站] 移入成功:" << fileId
                     << "affected:" << affectedCount
                     << "alreadyInTrash:" << alreadyInTrash;

            // 3. 构造智能提示语（already_in_trash 时静默不弹）
            if (!alreadyInTrash) {
                QString msg;
                if (affectedCount <= 1) {
                    msg = "文件已移入回收站";
                } else {
                    msg = QString("已将该文件夹及其 %1 个内容移入回收站")
                              .arg(affectedCount - 1);
                }
                emit trashSuccess(msg);
            }
        },
        // 失败回调
        [fileId](const QString &errMsg) {
            qWarning() << "[回收站] 移入失败:" << fileId << errMsg;
        }
    );
}

void FileController::moveSelectedToTrash()
{
    // 先收集所有选中的 fileId（避免遍历过程中 model 被修改导致索引错乱）
    QStringList selectedIds;
    for (int i = 0; i < m_fileModel->rowCount(); ++i) {
        QModelIndex idx = m_fileModel->index(i, 0);
        if (m_fileModel->data(idx, FileListModel::IsSelectedRole).toBool()) {
            selectedIds.append(m_fileModel->data(idx, FileListModel::FileIdRole).toString());
        }
    }

    qDebug() << "[回收站] 批量移入," << selectedIds.size() << "个文件";

    for (const QString &fid : selectedIds) {
        moveToTrash(fid);
    }
}
