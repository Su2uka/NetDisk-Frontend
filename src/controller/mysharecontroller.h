#ifndef MYSHARECONTROLLER_H
#define MYSHARECONTROLLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QVariantList>

// ── 分享项数据 ──
struct MyShareItem {
    int     shareId    = 0;
    QString shareKey;
    int     fileId     = 0;
    QString fileName;
    QString fileIcon;
    bool    isFolder   = false;
    bool    isSelected = false;
    int     viewCount  = 0;
    int     saveCount  = 0;
    QString createdAt;
    QString expireAt;    // 空 = 永久
    QString status;      // active / expired / cancelled
};

// ── 分享列表数据模型 ──
class MyShareModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        ShareIdRole = Qt::UserRole + 1,
        ShareKeyRole,
        FileIdRole,
        FileNameRole,
        FileIconRole,
        IsFolderRole,
        IsSelectedRole,
        ViewCountRole,
        SaveCountRole,
        CreatedAtRole,
        ExpireAtRole,
        StatusRole
    };
    Q_ENUM(Roles)

    explicit MyShareModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void loadData(const QVariantList &data);
    Q_INVOKABLE void toggleSelection(int row);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE int  selectedCount() const;
    Q_INVOKABLE bool hasSelection() const;
    Q_INVOKABLE QVariantMap getShareInfo(int index) const;
    Q_INVOKABLE void removeShareById(int shareId);

signals:
    void selectionChanged();
    void countChanged();

private:
    QList<MyShareItem> m_items;
};

// ── 我的分享控制器 ──
class MyShareController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyShareModel* shareModel READ shareModel CONSTANT)
    Q_PROPERTY(int  selectedCount READ selectedCount NOTIFY selectionStateChanged)
    Q_PROPERTY(bool hasSelection  READ hasSelection  NOTIFY selectionStateChanged)
    Q_PROPERTY(int  sortField     READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAsc       READ sortAsc   WRITE setSortAsc   NOTIFY sortAscChanged)
    Q_PROPERTY(bool hasMore       READ hasMore   NOTIFY hasMoreChanged)
    Q_PROPERTY(bool loadingMore   READ loadingMore NOTIFY loadingMoreChanged)

public:
    static MyShareController* instance();

    MyShareModel* shareModel() const { return m_model; }

    int  selectedCount() const;
    bool hasSelection()  const;
    int  sortField()     const { return m_sortField; }
    bool sortAsc()       const { return m_sortAsc; }
    bool hasMore()       const { return m_hasMore; }
    bool loadingMore()   const { return m_loadingMore; }

    void setSortField(int field);
    void setSortAsc(bool asc);

    // ── 操作 ──
    Q_INVOKABLE void loadShares();
    Q_INVOKABLE void loadMoreShares();
    Q_INVOKABLE void toggleSelection(int index);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void cancelShare(int shareId);
    Q_INVOKABLE void cancelSelected();
    Q_INVOKABLE void getShareCode(int index);

signals:
    void selectionStateChanged();
    void sortFieldChanged();
    void sortAscChanged();
    void hasMoreChanged();
    void loadingMoreChanged();
    void shareOpSuccess(const QString &message);
    void shareOpFailed(const QString &message);

private:
    explicit MyShareController(QObject *parent = nullptr);

    MyShareModel *m_model = nullptr;
    int  m_sortField   = 0;    // 0=created_at, 1=name, 2=view_count, 3=save_count
    bool m_sortAsc     = false;
    int  m_currentPage = 1;
    bool m_hasMore     = false;
    bool m_loadingMore = false;

    static QString fileIconForName(const QString &fileName, bool isFolder);
    static QString sortFieldToString(int field);
};

#endif // MYSHARECONTROLLER_H
