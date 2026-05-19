#ifndef RECYCLEBINCONTROLLER_H
#define RECYCLEBINCONTROLLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QVariantList>

// ── 回收站文件项 ──
struct RecycleBinItem {
    QString fileId;
    QString fileName;
    QString fileIcon;
    bool    isFolder   = false;
    bool    isSelected = false;
    qint64  fileSize   = 0;
    QString fileSizeStr;
    QString deletedAt;
};

// ── 回收站数据模型 ──
class RecycleBinModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        FileIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileIconRole,
        IsFolderRole,
        IsSelectedRole,
        FileSizeRole,
        FileSizeStrRole,
        DeletedAtRole
    };
    Q_ENUM(Roles)

    explicit RecycleBinModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void loadData(const QVariantList &data);
    Q_INVOKABLE void toggleSelection(int row);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE int  selectedCount() const;
    Q_INVOKABLE bool hasSelection() const;
    Q_INVOKABLE QVariantMap getFileInfo(int index) const;
    Q_INVOKABLE void removeFileById(const QString &fileId);

signals:
    void selectionChanged();
    void countChanged();

private:
    QList<RecycleBinItem> m_items;
};

// ── 回收站控制器 ──
class RecycleBinController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecycleBinModel* trashModel READ trashModel CONSTANT)
    Q_PROPERTY(int  selectedCount READ selectedCount NOTIFY selectionStateChanged)
    Q_PROPERTY(bool hasSelection  READ hasSelection  NOTIFY selectionStateChanged)
    Q_PROPERTY(int  viewMode      READ viewMode WRITE setViewMode NOTIFY viewModeChanged)

public:
    static RecycleBinController* instance();

    RecycleBinModel* trashModel() const { return m_model; }

    int  selectedCount() const;
    bool hasSelection()  const;
    int  viewMode()      const { return m_viewMode; }
    void setViewMode(int mode);

    // ── 操作 ──
    Q_INVOKABLE void loadTrash();
    Q_INVOKABLE void toggleSelection(int index);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void restoreFile(const QString &fileId);
    Q_INVOKABLE void restoreSelected();
    Q_INVOKABLE void restoreAll();
    Q_INVOKABLE void permanentDelete(const QString &fileId);
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void emptyTrash();

signals:
    void viewModeChanged();
    void selectionStateChanged();
    void trashOpSuccess(const QString &message);

private:
    explicit RecycleBinController(QObject *parent = nullptr);

    RecycleBinModel *m_model = nullptr;
    int m_viewMode = 0; // 0=grid, 1=list

    static QString fileIconForName(const QString &fileName, bool isFolder);
    static QString formatFileSize(qint64 bytes);
};

#endif // RECYCLEBINCONTROLLER_H
