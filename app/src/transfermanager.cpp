#include "transfermanager.h"

TransferManager::TransferManager(QObject *parent, OwnCloudBrowser *browser) :
    QObject(parent)
{
    this->browser = browser;
}

QVariantList TransferManager::getTransfers()
{
    downloadMutex.lock();
    uploadMutex.lock();

    // Show uploads before downloads in the list
    QVariantList transfers;

    foreach(TransferEntry *entry, uploadQueue) {
        QVariant tmpVariant;
        tmpVariant.setValue(entry);
        transfers.append(tmpVariant);
    }

    foreach(TransferEntry *entry, downloadQueue) {
        QVariant tmpVariant;
        tmpVariant.setValue(entry);
        transfers.append(tmpVariant);
    }

    downloadMutex.unlock();
    uploadMutex.unlock();

    return transfers;
}

bool TransferManager::isTransfering()
{
    return !downloadQueue.isEmpty() || !uploadQueue.isEmpty();
}

TransferEntry* TransferManager::enqueueDownload(EntryInfo* entry, bool open)
{
    downloadMutex.lock();

    QString name = entry->path().mid(entry->path().lastIndexOf("/") + 1);
    QString destination = destinationFromMIME(entry->mimeType()) + "/" + name;
    TransferEntry::TransferDirection direction = TransferEntry::DOWN;

    TransferEntry *newDownload = new TransferEntry(this,
                                                   browser->getWebdav(),
                                                   name,
                                                   entry->path(),
                                                   destination,
                                                   entry->size(),
                                                   direction,
                                                   open);

    connect(newDownload, SIGNAL(transferCompleted(bool)), this, SLOT(handleDownloadCompleted()), Qt::DirectConnection);
    if(downloadQueue.isEmpty()) {
        newDownload->startTransfer();
    }
    downloadQueue.enqueue(newDownload);

    downloadMutex.unlock();
    emit transferAdded();
    emit transferingChanged();

    return newDownload;
}

void TransferManager::enqueueUpload(QString localPath, QString remotePath)
{
    uploadMutex.lock();
    QFile localFile(localPath);
    qint64 size = localFile.size();

    QString name = localPath.mid(localPath.lastIndexOf("/") + 1);
    TransferEntry::TransferDirection direction = TransferEntry::UP;

    TransferEntry *newUpload = new TransferEntry(this,
                                                   browser->getWebdav(),
                                                   name,
                                                   remotePath,
                                                   localPath,
                                                   size,
                                                   direction);

    connect(newUpload, SIGNAL(transferCompleted(bool)), this, SLOT(handleUploadCompleted()), Qt::DirectConnection);
    if(uploadQueue.isEmpty()) {
        newUpload->startTransfer();
    }
    uploadQueue.enqueue(newUpload);
    uploadMutex.unlock();
    emit transferAdded();
    emit transferingChanged();
}

void TransferManager::handleDownloadCompleted()
{
    downloadMutex.lock();

    if(!downloadQueue.isEmpty()) {
        disconnect(downloadQueue.head(), SIGNAL(transferCompleted(bool)), this, SLOT(handleDownloadCompleted()));
        TransferEntry *entry = downloadQueue.dequeue();
        entry->deleteLater();
    }

    if(!downloadQueue.isEmpty() && downloadQueue.head())
        downloadQueue.head()->startTransfer();

    downloadMutex.unlock();
    emit transferingChanged();
}

void TransferManager::handleUploadCompleted()
{
    uploadMutex.lock();

    if(!uploadQueue.isEmpty()) {
        disconnect(uploadQueue.head(), SIGNAL(transferCompleted(bool)), this, SLOT(handleDownloadCompleted()));
        TransferEntry *entry = uploadQueue.dequeue();
        entry->deleteLater();
    }

    if(!uploadQueue.isEmpty() && uploadQueue.head())
        uploadQueue.head()->startTransfer();

    uploadMutex.unlock();
    emit transferingChanged();
}

bool TransferManager::isNotEnqueued(EntryInfo *entry)
{
    if(entry == NULL)
        return false;

    downloadMutex.lock();
    for(int i = 0; i < downloadQueue.size(); i++) {
        if(downloadQueue.at(i)->getRemotePath() == entry->path()) {
            return false;
        }
    }
    downloadMutex.unlock();

    return true;
}

QString TransferManager::destinationFromMIME(QString mime)
{
    QStandardPaths::StandardLocation location;

    if (mime.startsWith("image")) {
        location = QStandardPaths::PicturesLocation;
    } else if (mime.startsWith("video")) {
        location = QStandardPaths::MoviesLocation;
    } else if (mime.startsWith("audio")) {
        location = QStandardPaths::MusicLocation;
    } else {
        location = QStandardPaths::DownloadLocation;
    }

    return QStandardPaths::writableLocation(location);
}