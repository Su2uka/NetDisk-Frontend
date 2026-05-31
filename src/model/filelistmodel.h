#ifndef FILELISTMODEL_H
#define FILELISTMODEL_H

#include <QAbstractListModel>
#include <QVariantList>

struct FileItem {
    QString fileId;
    QString fileName;
    QString fileIcon;
    QString thumbnailUrl;
    bool isFolder;
    bool isSelected;
    QString createTime;
    QString modifyTime;
    qint64  fileSize = 0;
    QString fileSizeStr;
    QString parentId;
};

class FileListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum FileRoles {
        FileIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileIconRole,
        IsFolderRole,
        IsSelectedRole,
        CreateTimeRole,
        ModifyTimeRole,
        FileSizeRole,
        FileSizeStrRole,
        ParentIdRole,
        ThumbnailUrlRole
    };
    Q_ENUM(FileRoles)

    explicit FileListModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Methods
    Q_INVOKABLE void loadData(const QVariantList &data);
    Q_INVOKABLE void appendData(const QVariantList &data);
    Q_INVOKABLE void toggleSelection(int row);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE int selectedCount() const;
    Q_INVOKABLE bool hasSelection() const;
    Q_INVOKABLE QVariantMap getFileInfo(int index) const;
    Q_INVOKABLE void removeFileById(const QString &fileId);
    Q_INVOKABLE void updateFileName(const QString &fileId, const QString &newName);

signals:
    void selectionChanged();
    void countChanged();

private:
    QList<FileItem> m_items;
};

#endif // FILELISTMODEL_H
