#include "owncloudbrowser.h"

OwnCloudBrowser::OwnCloudBrowser(QObject *parent, Settings *settings) :
    QObject(parent)
{
    this->settings = settings;
    this->ignoreFail = true;

    // Need to decide on how to securely save password
    connect(settings, SIGNAL(settingsChanged()), this, SLOT(reloadSettings()));
    connect(&parser, SIGNAL(finished()), this, SLOT(handleResponse()));
    connect(&parser, SIGNAL(errorChanged(QString)), this, SLOT(printError(QString)));
    connect(&webdav, SIGNAL(errorChanged(QString)), this, SLOT(printError(QString)));
    connect(&webdav, SIGNAL(checkSslCertifcate(const QList<QSslError>&)), this, SLOT(proxyHandleSslError(const QList<QSslError>&)));
    connect(&webdav, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(proxyHandleLoginFailed()));
}

void OwnCloudBrowser::reloadSettings() {
    webdav.setConnectionSettings(settings->isHttps() ? QWebdav::HTTPS : QWebdav::HTTP,
                                 settings->hostname(),
                                 settings->path() + "/remote.php/webdav",
                                 settings->username(),
                                 settings->password(),
                                 settings->port(),
                                 settings->md5Hex(),
                                 settings->sha1Hex());
}

void OwnCloudBrowser::proxyHandleSslError(const QList<QSslError>& errors)
{
    QSslCertificate cert = errors[0].certificate();
    emit sslCertifcateError(webdav.digestToHex(cert.digest(QCryptographicHash::Md5)),
                            webdav.digestToHex(cert.digest(QCryptographicHash::Sha1)));
}

void OwnCloudBrowser::handleResponse()
{
    emit loginSucceeded();
    QList<QWebdavItem> list = parser.getList();
    QVariantList entries;

    QWebdavItem item;
    foreach(item, list) {
        EntryInfo *entry = new EntryInfo();
        entry->setName(item.name());
        entry->setDirectory(item.isDir());
        entry->setSize(item.size());
        if(!item.isDir())
            entry->setMimeType(item.mimeType());

        QVariant tmpVariant;
        tmpVariant.setValue(entry);
        entries.append(tmpVariant);
    }
    emit directoryContentChanged(currentPath, entries);
}

void OwnCloudBrowser::printError(QString msg)
{
    qDebug() << "ERROR: " << msg;
}

QString OwnCloudBrowser::getCurrentPath()
{
    return currentPath;
}

void OwnCloudBrowser::getDirectoryContent(QString path)
{
    currentPath = path;
    parser.listDirectory(&webdav, path);
}

void OwnCloudBrowser::proxyHandleLoginFailed()
{
    if(ignoreFail) {
        ignoreFail = false;
        getDirectoryContent("/");
    } else {
        emit loginFailed();
    }
}