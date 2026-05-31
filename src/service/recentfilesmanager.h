#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonObject>
#include <QObject>

struct RecentFileRecord {
    QString fileId;
    QString parentId;
    QString fileName;
    QString fileIcon;
    QString thumbnailUrl;
    QString source;
    QDateTime accessedAt;

    QJsonObject toJson() const;
    static RecentFileRecord fromJson(const QJsonObject &obj);
};

class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles : int {
        FileIdRole = Qt::UserRole + 1,
        ParentIdRole,
        FileNameRole,
        FileIconRole,
        ThumbnailUrlRole,
        FileTimeRole,
        SourceRole,
        AccessedAtRole,
    };
    Q_ENUM(Roles)

    explicit RecentFilesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRecords(const QList<RecentFileRecord> &records);
    void upsert(const RecentFileRecord &record, int maxCount);
    void clear();

    const QList<RecentFileRecord>& records() const;

signals:
    void countChanged();

private:
    static QString displayTime(const QDateTime &time);

    QList<RecentFileRecord> m_records;
};

class RecentFilesManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecentFilesModel* model READ model CONSTANT)

public:
    static RecentFilesManager* instance();

    RecentFilesModel* model() const;

    Q_INVOKABLE void addRecentFile(const QString &fileId,
                                   const QString &parentId,
                                   const QString &fileName,
                                   const QString &source = QString());
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void reloadForCurrentUser();

    void connectControllers();

private:
    explicit RecentFilesManager(QObject *parent = nullptr);

    static QString fileIconForName(const QString &fileName);
    static QString thumbnailUrlForFile(const QString &fileId, const QString &fileName);
    QString recentFilePath() const;
    void loadFromDisk();
    void saveToDisk() const;

    RecentFilesModel *m_model = nullptr;
    bool m_connected = false;
};

#endif // RECENTFILESMANAGER_H
