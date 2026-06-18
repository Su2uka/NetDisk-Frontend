#ifndef FILECONTROLLER_H
#define FILECONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QJsonArray>
#include "../model/filelistmodel.h"

class FileController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FileListModel* fileModel READ fileModel CONSTANT)
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QVariantList breadcrumbs READ breadcrumbs NOTIFY breadcrumbsChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionStateChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionStateChanged)
    Q_PROPERTY(int sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAsc READ sortAsc WRITE setSortAsc NOTIFY sortAscChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(bool loadingMore READ loadingMore NOTIFY loadingMoreChanged)
    Q_PROPERTY(bool searchMode READ searchMode NOTIFY searchModeChanged)
    Q_PROPERTY(QString searchKeyword READ searchKeyword NOTIFY searchKeywordChanged)

public:
    static FileController* instance();

    FileListModel* fileModel() const;
    int viewMode() const;
    void setViewMode(int mode);
    QVariantList breadcrumbs() const;
    QString currentFolderId() const;
    
    int selectedCount() const;
    bool hasSelection() const;
    
    int sortField() const;
    void setSortField(int field);
    bool sortAsc() const;
    void setSortAsc(bool asc);

    bool loading() const;
    bool hasMore() const;
    bool loadingMore() const;
    bool searchMode() const;
    QString searchKeyword() const;

    // ── QML Invokables ──

    // Navigation
    Q_INVOKABLE void enterFolder(const QString &folderId, const QString &title);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void navigateTo(int index);

    // Selection
    Q_INVOKABLE void toggleSelection(int index);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();

    // 从后端加载当前目录的文件列表
    Q_INVOKABLE void loadFiles();
    Q_INVOKABLE void loadMoreFiles();

    // 按分类加载所有文件（图片/文档/视频/音频），结果缓存到本地
    Q_INVOKABLE void loadFilesByCategory(const QString &category);

    // 从已缓存的分类数据中按扩展名过滤（客户端筛选，无网络请求）
    Q_INVOKABLE void filterCategoryByExt(const QString &ext);

    // Search
    Q_INVOKABLE void searchFiles(const QString &keyword);
    Q_INVOKABLE void searchSuggestions(const QString &keyword);
    Q_INVOKABLE void clearSearch();

    // Upload（职责：仅将任务委派给 UploadController 队列，不做任何 I/O）
    Q_INVOKABLE void uploadFiles(const QList<QUrl> &fileUrls);
    Q_INVOKABLE void uploadFolder(const QUrl &folderUrl);

    // Download（向后端换取预签名直链，然后委派给 DownloadController 队列）
    Q_INVOKABLE void requestDownload(const QString &fileId,
                                      const QString &parentId,
                                      const QString &fileName,
                                      qint64 fileSize,
                                      const QString &saveDirPath);

    // Download Folder
    Q_INVOKABLE void downloadFolder(const QString &folderId,
                                     const QString &folderName,
                                     const QString &saveDirPath);

    // Trash（加入回收站）
    Q_INVOKABLE void moveToTrash(const QString &fileId);
    Q_INVOKABLE void moveSelectedToTrash();

    // 新建文件夹
    Q_INVOKABLE void createFolder(const QString &folderName);

    // 重命名文件/文件夹
    Q_INVOKABLE void renameItem(const QString &fileId, const QString &newName);

    // 前往文件位置（从智能目录跳转到文件所在文件夹）
    Q_INVOKABLE void navigateToFileLocation(const QString &parentId);

signals:
    void viewModeChanged();
    void breadcrumbsChanged();
    void selectionStateChanged();
    void sortFieldChanged();
    void sortAscChanged();
    void loadingChanged();
    void hasMoreChanged();
    void loadingMoreChanged();
    void searchModeChanged();
    void searchKeywordChanged();
    void searchSuggestionsReady(const QString &keyword, const QVariantList &results);
    void searchFailed(const QString &message);
    void trashSuccess(const QString &message);
    void folderCreated(const QString &message);
    void renameSuccess();
    void operationFailed(const QString &message);

    // 通知 MainPage 导航到文件位置
    void goToFileLocationRequested(const QString &folderId);

private:
    explicit FileController(QObject *parent = nullptr);

    /// 递归串行获取每个文件的预签名 URL，全部完成后批量入队
    void fetchDownloadUrlsAndEnqueue(const QJsonArray &files,
                                      const QString &localRootPath,
                                      int index,
                                      QVariantList *accumulatedResults);

    /// 根据文件名后缀返回对应的文件类型图标路径
    static QString fileIconForName(const QString &fileName, bool isFolder);
    static QString thumbnailUrlForFile(const QString &fileId, const QString &fileName, bool isFolder);

    /// 将字节数格式化为可读字符串（KB/MB/GB）
    static QString formatFileSize(qint64 bytes);

    QVariantMap fileItemFromJson(const QJsonObject &obj) const;
    QJsonObject baseSearchParams(const QString &keyword, int page, int pageSize) const;
    void loadMoreSearchFiles();

    FileListModel* m_fileModel;
    int m_viewMode = 0;
    QVariantList m_breadcrumbs;
    int m_sortField = 1;
    bool m_sortAsc = false;
    bool m_loading = false;
    bool m_hasMore = false;              // 是否还有更多数据
    bool m_loadingMore = false;          // 正在加载下一页
    int  m_currentPage = 1;              // 当前已加载的页码
    static const int PAGE_SIZE = 50;     // 每页条数
    bool m_searchMode = false;
    QString m_searchKeyword;
    QVariantList m_categoryCache;  // 分类全量缓存，供客户端筛选
    QVariantList m_pendingBreadcrumbs;  // 「前往文件位置」待导航的完整面包屑链
};

#endif // FILECONTROLLER_H
