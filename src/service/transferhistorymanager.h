#ifndef TRANSFERHISTORYMANAGER_H
#define TRANSFERHISTORYMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

// ── 单条传输完成记录 ──
struct TransferRecord {
    QString  taskId;
    QString  fileName;
    qint64   fileSize;
    bool     isUpload;    // true=上传, false=下载
    int      status;      // 3=Success, 4=Failed, 5=Canceled
    QString  errorMsg;
    QDateTime finishedAt;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["taskId"]     = taskId;
        obj["fileName"]   = fileName;
        obj["fileSize"]   = fileSize;
        obj["isUpload"]   = isUpload;
        obj["status"]     = status;
        obj["errorMsg"]   = errorMsg;
        obj["finishedAt"] = finishedAt.toString(Qt::ISODate);
        return obj;
    }

    static TransferRecord fromJson(const QJsonObject &obj) {
        TransferRecord r;
        r.taskId     = obj["taskId"].toString();
        r.fileName   = obj["fileName"].toString();
        r.fileSize   = obj["fileSize"].toDouble();
        r.isUpload   = obj["isUpload"].toBool();
        r.status     = obj["status"].toInt();
        r.errorMsg   = obj["errorMsg"].toString();
        r.finishedAt = QDateTime::fromString(obj["finishedAt"].toString(), Qt::ISODate);
        return r;
    }
};


// ── 历史记录列表模型 ──
class TransferHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        TaskIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileSizeRole,
        IsUploadRole,
        StatusRole,
        ErrorMsgRole,
        FinishedAtRole
    };

    explicit TransferHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendRecord(const TransferRecord &record);
    void clear();

    const QList<TransferRecord>& records() const { return m_records; }

signals:
    void countChanged();

private:
    QList<TransferRecord> m_records;
};


// ── 管理器单例 ──
class TransferHistoryManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TransferHistoryModel* model READ model CONSTANT)

public:
    static TransferHistoryManager* instance();

    TransferHistoryModel* model() const;

    /// 初始化：连接 UploadController / DownloadController 的信号
    void connectControllers();

    /// 添加一条记录并持久化
    Q_INVOKABLE void addRecord(const QString &taskId,
                                const QString &fileName,
                                qint64 fileSize,
                                bool isUpload,
                                int status,
                                const QString &errorMsg = QString());

    /// 清空所有历史记录
    Q_INVOKABLE void clearAll();

    /// 切换账号后从当前用户目录重新加载历史记录
    Q_INVOKABLE void reloadForCurrentUser();

private:
    explicit TransferHistoryManager(QObject *parent = nullptr);

    void loadFromDisk();
    void saveToDisk();
    QString historyFilePath() const;

    TransferHistoryModel *m_model = nullptr;
};

#endif // TRANSFERHISTORYMANAGER_H
