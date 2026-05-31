#include "settingscontroller.h"
#include "uploadcontroller.h"
#include "downloadcontroller.h"
#include <QSettings>
#include <QStandardPaths>

// ── 单例 ──
SettingsController* SettingsController::instance()
{
    static SettingsController inst;
    return &inst;
}

SettingsController::SettingsController(QObject *parent)
    : QObject{parent}
{
    QSettings s;
    s.beginGroup("Settings");

    m_downloadDir = s.value("downloadDir",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    m_maxUploadConcurrent = s.value("maxUploadConcurrent", 3).toInt();
    m_maxDownloadConcurrent = s.value("maxDownloadConcurrent", 3).toInt();
    m_autoStartUpload = s.value("autoStartUpload", true).toBool();
    m_autoStartDownload = s.value("autoStartDownload", true).toBool();

    s.endGroup();

    // 同步到上传/下载控制器
    UploadController::instance()->setMaxConcurrent(m_maxUploadConcurrent);
    DownloadController::instance()->setMaxConcurrent(m_maxDownloadConcurrent);
}

// ── 下载目录 ──
QString SettingsController::downloadDir() const
{
    return m_downloadDir;
}

void SettingsController::setDownloadDir(const QString &dir)
{
    if (m_downloadDir == dir)
        return;
    m_downloadDir = dir;
    QSettings s;
    s.setValue("Settings/downloadDir", dir);
    emit downloadDirChanged();
}

// ── 上传并发数 ──
int SettingsController::maxUploadConcurrent() const
{
    return m_maxUploadConcurrent;
}

void SettingsController::setMaxUploadConcurrent(int n)
{
    n = qBound(1, n, 3);
    if (m_maxUploadConcurrent == n)
        return;
    m_maxUploadConcurrent = n;
    QSettings s;
    s.setValue("Settings/maxUploadConcurrent", n);
    UploadController::instance()->setMaxConcurrent(n);
    emit maxUploadConcurrentChanged();
}

// ── 下载并发数 ──
int SettingsController::maxDownloadConcurrent() const
{
    return m_maxDownloadConcurrent;
}

void SettingsController::setMaxDownloadConcurrent(int n)
{
    n = qBound(1, n, 3);
    if (m_maxDownloadConcurrent == n)
        return;
    m_maxDownloadConcurrent = n;
    QSettings s;
    s.setValue("Settings/maxDownloadConcurrent", n);
    DownloadController::instance()->setMaxConcurrent(n);
    emit maxDownloadConcurrentChanged();
}

// ── 自动开始上传 ──
bool SettingsController::autoStartUpload() const
{
    return m_autoStartUpload;
}

void SettingsController::setAutoStartUpload(bool on)
{
    if (m_autoStartUpload == on)
        return;
    m_autoStartUpload = on;
    QSettings s;
    s.setValue("Settings/autoStartUpload", on);
    emit autoStartUploadChanged();
}

// ── 自动开始下载 ──
bool SettingsController::autoStartDownload() const
{
    return m_autoStartDownload;
}

void SettingsController::setAutoStartDownload(bool on)
{
    if (m_autoStartDownload == on)
        return;
    m_autoStartDownload = on;
    QSettings s;
    s.setValue("Settings/autoStartDownload", on);
    emit autoStartDownloadChanged();
}