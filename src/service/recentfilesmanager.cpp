#include "recentfilesmanager.h"

#include "userstoragepaths.h"
#include "../controller/downloadcontroller.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QDebug>
#include <QSaveFile>
#include <QStringList>

namespace {
constexpr int kMaxRecentFiles = 10;
}

QJsonObject RecentFileRecord::toJson() const
{
    QJsonObject obj;
    obj["fileId"] = fileId;
    obj["parentId"] = parentId;
    obj["fileName"] = fileName;
    obj["fileIcon"] = fileIcon;
    obj["thumbnailUrl"] = thumbnailUrl;
    obj["source"] = source;
    obj["accessedAt"] = accessedAt.toString(Qt::ISODate);
    return obj;
}

RecentFileRecord RecentFileRecord::fromJson(const QJsonObject &obj)
{
    RecentFileRecord record;
    record.fileId = obj["fileId"].toString();
    record.parentId = obj["parentId"].toString();
    record.fileName = obj["fileName"].toString();
    record.fileIcon = obj["fileIcon"].toString();
    record.thumbnailUrl = obj["thumbnailUrl"].toString();
    record.source = obj["source"].toString();
    record.accessedAt = QDateTime::fromString(obj["accessedAt"].toString(), Qt::ISODate);
    return record;
}

RecentFilesModel::RecentFilesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RecentFilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_records.size();
}

QVariant RecentFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size())
        return {};

    const RecentFileRecord &record = m_records.at(index.row());
    switch (role) {
    case FileIdRole: return record.fileId;
    case ParentIdRole: return record.parentId;
    case FileNameRole: return record.fileName;
    case FileIconRole: return record.fileIcon;
    case ThumbnailUrlRole: return record.thumbnailUrl;
    case FileTimeRole: return displayTime(record.accessedAt);
    case SourceRole: return record.source;
    case AccessedAtRole: return record.accessedAt.toString(Qt::ISODate);
    }

    return {};
}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {FileIdRole, "fileId"},
        {ParentIdRole, "parentId"},
        {FileNameRole, "fileName"},
        {FileIconRole, "fileIcon"},
        {ThumbnailUrlRole, "thumbnailUrl"},
        {FileTimeRole, "fileTime"},
        {SourceRole, "source"},
        {AccessedAtRole, "accessedAt"},
    };
    return roles;
}

void RecentFilesModel::setRecords(const QList<RecentFileRecord> &records)
{
    const int oldCount = m_records.size();
    beginResetModel();
    m_records = records;
    endResetModel();
    if (oldCount != m_records.size())
        emit countChanged();
}

void RecentFilesModel::upsert(const RecentFileRecord &record, int maxCount)
{
    QList<RecentFileRecord> next = m_records;
    for (int i = next.size() - 1; i >= 0; --i) {
        const bool sameId = !record.fileId.isEmpty() && next.at(i).fileId == record.fileId;
        const bool sameName = record.fileId.isEmpty() && next.at(i).fileName == record.fileName;
        if (sameId || sameName)
            next.removeAt(i);
    }

    next.prepend(record);
    while (next.size() > maxCount)
        next.removeLast();
    setRecords(next);
}

void RecentFilesModel::clear()
{
    setRecords({});
}

const QList<RecentFileRecord>& RecentFilesModel::records() const
{
    return m_records;
}

QString RecentFilesModel::displayTime(const QDateTime &time)
{
    if (!time.isValid())
        return {};

    const QDateTime localTime = time.toLocalTime();
    const QDate today = QDate::currentDate();
    const QDate date = localTime.date();
    if (date == today)
        return QString("今天 %1").arg(localTime.toString("HH:mm"));
    if (date == today.addDays(-1))
        return QString("昨天 %1").arg(localTime.toString("HH:mm"));
    return localTime.toString("yyyy-MM-dd HH:mm");
}

RecentFilesManager* RecentFilesManager::instance()
{
    static RecentFilesManager inst;
    return &inst;
}

RecentFilesManager::RecentFilesManager(QObject *parent)
    : QObject(parent)
    , m_model(new RecentFilesModel(this))
{
    loadFromDisk();
}

RecentFilesModel* RecentFilesManager::model() const
{
    return m_model;
}

void RecentFilesManager::addRecentFile(const QString &fileId,
                                       const QString &parentId,
                                       const QString &fileName,
                                       const QString &source)
{
    const QString trimmedId = fileId.trimmed();
    const QString trimmedName = fileName.trimmed();
    if (trimmedId.isEmpty() || trimmedName.isEmpty())
        return;

    RecentFileRecord record;
    record.fileId = trimmedId;
    record.parentId = parentId.trimmed();
    record.fileName = trimmedName;
    record.fileIcon = fileIconForName(trimmedName);
    record.thumbnailUrl = thumbnailUrlForFile(trimmedId, trimmedName);
    record.source = source;
    record.accessedAt = QDateTime::currentDateTimeUtc();

    m_model->upsert(record, kMaxRecentFiles);
    saveToDisk();
}

void RecentFilesManager::clearAll()
{
    m_model->clear();
    saveToDisk();
}

void RecentFilesManager::reloadForCurrentUser()
{
    loadFromDisk();
}

void RecentFilesManager::connectControllers()
{
    if (m_connected)
        return;
    m_connected = true;

    auto *download = DownloadController::instance();
    connect(download, &DownloadController::taskSuccess, this,
            [this, download](const QString &taskId, const QString &fileName) {
        QString fileId;
        QString parentId;
        const auto &tasks = download->model()->tasks();
        for (const auto &task : tasks) {
            if (task.taskId == taskId) {
                if (!task.parentIdKnown)
                    return;
                fileId = task.fileId;
                parentId = task.parentId;
                break;
            }
        }
        addRecentFile(fileId, parentId, fileName, "download");
    });
}

QString RecentFilesManager::fileIconForName(const QString &fileName)
{
    const QString prefix = "qrc:/file_type/res/file_type/";
    const QString suffix = QFileInfo(fileName).suffix().toLower();

    if (QStringList{"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"}.contains(suffix))
        return prefix + "ft-video.svg";
    if (QStringList{"mp3", "wav", "flac", "aac", "ogg", "m4a"}.contains(suffix))
        return prefix + "ft-audio.svg";
    if (QStringList{"jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"}.contains(suffix))
        return prefix + "ft-image.svg";
    if (suffix == "pdf")
        return prefix + "ft-pdf.svg";
    if (suffix == "doc" || suffix == "docx")
        return prefix + "ft-word.svg";
    if (suffix == "xls" || suffix == "xlsx")
        return prefix + "ft-excel.svg";
    if (suffix == "ppt" || suffix == "pptx")
        return prefix + "ft-ppt.svg";
    if (QStringList{"zip", "rar", "7z", "tar", "gz"}.contains(suffix))
        return prefix + "ft-archive.svg";
    if (QStringList{"txt", "md", "log", "ini"}.contains(suffix))
        return prefix + "ft-text.svg";
    if (QStringList{"cpp", "h", "hpp", "py", "js", "ts", "qml", "json", "xml", "html", "css"}.contains(suffix))
        return prefix + "ft-code.svg";
    if (suffix == "psd" || suffix == "ai")
        return prefix + "ft-psd.svg";
    return prefix + "ft-unknown.svg";
}

QString RecentFilesManager::thumbnailUrlForFile(const QString &fileId, const QString &fileName)
{
    if (fileId.isEmpty())
        return QString();

    const QString suffix = QFileInfo(fileName).suffix().toLower();
    static const QStringList imageSuffixes = {
        "jpg", "jpeg", "png", "gif", "bmp", "webp", "tiff", "tif"
    };

    if (imageSuffixes.contains(suffix))
        return "image://thumbnail/" + fileId;

    return QString();
}

QString RecentFilesManager::recentFilePath() const
{
    return UserStoragePaths::userDataPath("recent_files.json");
}

void RecentFilesManager::loadFromDisk()
{
    QFile file(recentFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        m_model->clear();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "[最近文件] 本地记录解析失败:" << parseError.errorString();
        return;
    }

    QList<RecentFileRecord> records;
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr) {
        if (!value.isObject())
            continue;
        RecentFileRecord record = RecentFileRecord::fromJson(value.toObject());
        if (record.fileIcon.isEmpty())
            record.fileIcon = fileIconForName(record.fileName);
        if (record.thumbnailUrl.isEmpty())
            record.thumbnailUrl = thumbnailUrlForFile(record.fileId, record.fileName);
        if (!record.fileId.isEmpty() && !record.fileName.isEmpty())
            records.append(record);
        if (records.size() >= kMaxRecentFiles)
            break;
    }

    m_model->setRecords(records);
}

void RecentFilesManager::saveToDisk() const
{
    QJsonArray arr;
    for (const auto &record : m_model->records())
        arr.append(record.toJson());

    QSaveFile file(recentFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QByteArray data = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    if (file.write(data) != data.size()) {
        qWarning() << "[最近文件] 写入本地记录失败:" << file.errorString();
        return;
    }
    if (!file.commit()) {
        qWarning() << "[最近文件] 提交本地记录失败:" << file.errorString();
        return;
    }
}
