#ifndef PREVIEWCONTROLLER_H
#define PREVIEWCONTROLLER_H

#include <QObject>

class PreviewController : public QObject
{
    Q_OBJECT

public:
    explicit PreviewController(QObject *parent = nullptr);

    Q_INVOKABLE void previewFile(const QString &fileId,
                                 const QString &parentId,
                                 const QString &fileName,
                                 bool isFolder);

signals:
    void previewReady(const QString &previewUrl,
                      const QString &mediaType,
                      const QString &mimeType,
                      const QString &fileName);
    void previewFailed(const QString &message);
};

#endif // PREVIEWCONTROLLER_H
