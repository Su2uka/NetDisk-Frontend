#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>

class SettingsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString downloadDir READ downloadDir WRITE setDownloadDir NOTIFY downloadDirChanged)
    Q_PROPERTY(int maxUploadConcurrent READ maxUploadConcurrent WRITE setMaxUploadConcurrent NOTIFY maxUploadConcurrentChanged)
    Q_PROPERTY(int maxDownloadConcurrent READ maxDownloadConcurrent WRITE setMaxDownloadConcurrent NOTIFY maxDownloadConcurrentChanged)
    Q_PROPERTY(bool autoStartUpload READ autoStartUpload WRITE setAutoStartUpload NOTIFY autoStartUploadChanged)
    Q_PROPERTY(bool autoStartDownload READ autoStartDownload WRITE setAutoStartDownload NOTIFY autoStartDownloadChanged)

public:
    static SettingsController* instance();

    QString downloadDir() const;
    void setDownloadDir(const QString &dir);

    int maxUploadConcurrent() const;
    void setMaxUploadConcurrent(int n);

    int maxDownloadConcurrent() const;
    void setMaxDownloadConcurrent(int n);

    bool autoStartUpload() const;
    void setAutoStartUpload(bool on);

    bool autoStartDownload() const;
    void setAutoStartDownload(bool on);

signals:
    void downloadDirChanged();
    void maxUploadConcurrentChanged();
    void maxDownloadConcurrentChanged();
    void autoStartUploadChanged();
    void autoStartDownloadChanged();

private:
    explicit SettingsController(QObject *parent = nullptr);

    QString m_downloadDir;
    int m_maxUploadConcurrent = 3;
    int m_maxDownloadConcurrent = 3;
    bool m_autoStartUpload = true;
    bool m_autoStartDownload = true;
};

#endif // SETTINGSCONTROLLER_H