#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

namespace AppConfig {
    // 应用程序信息
    const QString ORGANIZATION_NAME = "AeroDriveOrg";
    const QString ORGANIZATION_DOMAIN = "aerodrive.cloud";
    const QString APPLICATION_NAME = "AeroDriveApp";

    // API 基础路径配置
#ifdef QT_DEBUG
    // 开发环境
    const QString API_BASE = "http://localhost:8000/api/v1";
#else
    // 生产环境
    const QString API_BASE = "http://localhost:8080/api";
#endif

    // 网络超时时间 (毫秒)
    const int NETWORK_TIMEOUT = 10000;
}

#endif // APPCONFIG_H
