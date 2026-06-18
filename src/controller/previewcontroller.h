#ifndef PREVIEWCONTROLLER_H
#define PREVIEWCONTROLLER_H

#include <QObject>

class PreviewController : public QObject
{
    Q_OBJECT

public:
    static PreviewController* instance();

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

private:
    explicit PreviewController(QObject *parent = nullptr);
};

#endif // PREVIEWCONTROLLER_H
