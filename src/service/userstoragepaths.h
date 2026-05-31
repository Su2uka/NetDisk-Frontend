#ifndef USERSTORAGEPATHS_H
#define USERSTORAGEPATHS_H

#include <QString>

namespace UserStoragePaths {

QString currentUserKey();
QString userDataDir();
QString userDataPath(const QString &fileName);
QString userCacheDir(const QString &subDir = QString());

}

#endif // USERSTORAGEPATHS_H
