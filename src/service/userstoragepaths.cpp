#include "userstoragepaths.h"

#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace {

QString safePathPart(QString value)
{
    value = value.trimmed();
    if (value.isEmpty())
        value = "anonymous";
    value.replace(QRegularExpression("[^A-Za-z0-9_.-]"), "_");
    return value;
}

QString ensureDir(const QString &dir)
{
    QDir().mkpath(dir);
    return dir;
}

}

namespace UserStoragePaths {

QString currentUserKey()
{
    QSettings settings;
    const QString userId = settings.value("user/current_user_id").toString();
    if (!userId.isEmpty())
        return safePathPart("user_" + userId);
    return "anonymous";
}

QString userDataDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return ensureDir(base + "/users/" + currentUserKey());
}

QString userDataPath(const QString &fileName)
{
    return userDataDir() + "/" + fileName;
}

QString userCacheDir(const QString &subDir)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                  + "/users/" + currentUserKey();
    if (!subDir.isEmpty())
        dir += "/" + subDir;
    return ensureDir(dir);
}

}
