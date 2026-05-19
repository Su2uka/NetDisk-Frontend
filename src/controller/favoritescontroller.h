#ifndef FAVORITESCONTROLLER_H
#define FAVORITESCONTROLLER_H

#include <QObject>
#include "../model/filelistmodel.h"

class FavoritesController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FileListModel* fileModel READ fileModel CONSTANT)
    Q_PROPERTY(int  selectedCount READ selectedCount NOTIFY selectionStateChanged)
    Q_PROPERTY(bool hasSelection  READ hasSelection  NOTIFY selectionStateChanged)
    Q_PROPERTY(int  viewMode      READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(int  sortField     READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAsc       READ sortAsc   WRITE setSortAsc   NOTIFY sortAscChanged)
    Q_PROPERTY(bool hasMore       READ hasMore   NOTIFY hasMoreChanged)
    Q_PROPERTY(bool loadingMore   READ loadingMore NOTIFY loadingMoreChanged)

public:
    static FavoritesController* instance();

    FileListModel* fileModel() const { return m_model; }

    int  selectedCount() const;
    bool hasSelection()  const;
    int  viewMode()      const { return m_viewMode; }
    void setViewMode(int mode);
    int  sortField()     const { return m_sortField; }
    void setSortField(int field);
    bool sortAsc()       const { return m_sortAsc; }
    void setSortAsc(bool asc);
    bool hasMore()       const { return m_hasMore; }
    bool loadingMore()   const { return m_loadingMore; }

    // ── 操作 ──
    Q_INVOKABLE void loadFavorites();
    Q_INVOKABLE void loadMoreFavorites();
    Q_INVOKABLE void toggleSelection(int index);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void addFavorite(const QString &fileId);
    Q_INVOKABLE void removeFavorite(const QString &fileId);
    Q_INVOKABLE void removeSelectedFavorites();

signals:
    void viewModeChanged();
    void selectionStateChanged();
    void sortFieldChanged();
    void sortAscChanged();
    void hasMoreChanged();
    void loadingMoreChanged();
    void favOpSuccess(const QString &message);
    void favOpFailed(const QString &message);

private:
    explicit FavoritesController(QObject *parent = nullptr);

    FileListModel *m_model = nullptr;
    int  m_viewMode    = 0;    // 0=grid, 1=list
    int  m_sortField   = 1;    // 0=name, 1=created_at, 2=updated_at, 3=size
    bool m_sortAsc     = false;
    int  m_currentPage = 1;
    bool m_hasMore     = false;
    bool m_loadingMore = false;
    static const int PAGE_SIZE = 50;

    static QString fileIconForName(const QString &fileName, bool isFolder);
    static QString formatFileSize(qint64 bytes);
};

#endif // FAVORITESCONTROLLER_H
