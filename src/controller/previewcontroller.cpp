#include "previewcontroller.h"

#include <QJsonObject>
#include <QPointer>

#include "../network/networkmanager.h"
#include "../service/recentfilesmanager.h"

PreviewController::PreviewController(QObject *parent)
    : QObject{parent}
{
}

void PreviewController::previewFile(const QString &fileId,
                                    const QString &parentId,
                                    const QString &fileName,
                                    bool isFolder)
{
    if (isFolder || fileId.trimmed().isEmpty()) {
        emit previewFailed("仅支持预览图片、视频和音频文件");
        return;
    }

    QPointer<PreviewController> self(this);

    NetworkManager::instance()->get(
        "/files/" + fileId + "/preview_url",
        QJsonObject(),
        [self, fileId, parentId, fileName](const QJsonObject &data) {
            if (!self)
                return;

            QString previewUrl = data["preview_url"].toString();
            QString mediaType = data["media_type"].toString();
            QString mimeType = data["mime_type"].toString();
            QString resolvedFileName = data["file_name"].toString();

            if (previewUrl.isEmpty() || mediaType.isEmpty()) {
                emit self->previewFailed("预览地址返回异常");
                return;
            }

            if (resolvedFileName.isEmpty())
                resolvedFileName = fileName;

            RecentFilesManager::instance()->addRecentFile(fileId, parentId, resolvedFileName, "preview");
            emit self->previewReady(previewUrl, mediaType, mimeType, resolvedFileName);
        },
        [self](const QString &errMsg) {
            if (!self)
                return;
            emit self->previewFailed(errMsg);
        }
    );
}
