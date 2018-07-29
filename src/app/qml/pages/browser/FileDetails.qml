import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.owncloud 1.0
import SailfishUiSet 1.0
import QtMultimedia 5.6

Page {
    id: pageRoot
    anchors.fill: parent

    property CommandEntity downloadCommand : null
    property var entry : null;
    readonly property bool isDownloading : downloadCommand !== null ||
                                  applicationWindow.isTransferEnqueued(entry.path);
    readonly property string imgSrc :
        (!thumbnailFetcher.fetching && thumbnailFetcher.source !== "") ?
            thumbnailFetcher.source :
            fileDetailsHelper.getIconFromMime(entry.mimeType)
    readonly property bool isAudioVideo :
        (entry.mimeType.indexOf("video") === 0 ||
         entry.mimeType.indexOf("audio") === 0)

    Connections {
        target: downloadCommand
        onDone: downloadCommand = null
        onAborted: downloadCommand = null
    }

    Component.onCompleted: {
        if (isAudioVideo)
            return;

        console.log("Fetching thumbnail: " + entry.path)
        thumbnailFetcher.fetchThumbnail(entry.path);
    }
    Connections {
        target: transferQueue
        onCommandFinished: {
            // Invalidate downloadEntry after completion
            if (receipt.info.property("type") !== "fileDownload" ||
                    receipt.info.property("type") !== "fileUpload") {
                return;
            }

            if (receipt.info.property("remotePath") === pageRoot.remotePath) {
                isDownloading = false
            }
        }
    }

    FileDetailsHelper { id: fileDetailsHelper }

    function startDownload(path, mimeType, open) {
        downloadCommand = transferQueue.fileDownloadRequest(path, mimeType, open)
        transferQueue.run()
    }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                id: download
                text: qsTr("Download")
                enabled: !isDownloading
                onClicked: {
                    startDownload(entry.path, entry.mimeType, false)
                }
            }

            MenuItem {
                id: downloadAndOpen
                text: qsTr("Download and open")
                enabled: !isDownloading
                onClicked: {
                    startDownload(entry.path, entry.mimeType, true)
                }
            }
        }

        PageHeader {
            id: header
            title: qsTr("Details")
        }

        ThumbnailFetcher {
            id: thumbnailFetcher
            settings: persistentSettings
            commandQueue: browserCommandQueue
            width: fileImage.width
            height: fileImage.height
        }

        // Icon & Progress spinner
        Item {
            property int margins : parent.width / 8

            id: fileImage
            height: width
            anchors.top: header.bottom
            anchors.topMargin: margins
            anchors.left: parent.left
            anchors.leftMargin: margins
            anchors.right: parent.right
            anchors.rightMargin: margins

            // Image thumbnail
            Image {
                id: thumbnailView
                source: isAudioVideo ? "" : imgSrc
                visible: !isAudioVideo
                anchors.fill: parent
                asynchronous: true
                onSourceChanged: {
                    console.log("thumbnail source " + source)
                }
            }

            // Media player for video and audio preview
            WebDavMediaFeeder {
                id: mediaFeeder
                mediaPlayer: previewPlayer
                settings: persistentSettings
                url: isAudioVideo ?
                            (FilePathUtil.getWebDavFileUrl(entry.path, persistentSettings)) :
                            ""
            }
            MediaPlayer {
                id: previewPlayer
                autoPlay: false
                onSourceChanged: {
                    console.log("AV preview " + source)
                }
                onPlaybackStateChanged: {
                    console.log("playback state " + playbackState)
                }
            }
            VideoOutput {
                id: mediaView
                source: previewPlayer
                visible: isAudioVideo
                anchors.fill: parent
            }
            Image {
                anchors.centerIn: parent
                source: (previewPlayer.playbackState == MediaPlayer.PlayingState) ?
                            "image://theme/icon-l-pause" :
                            "image://theme/icon-l-play"
                visible: mediaView.visible
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("clicked @ " + previewPlayer.playbackState)
                        if (previewPlayer.playbackState == MediaPlayer.PlayingState) {
                            mediaFeeder.pause()
                        } else {
                            mediaFeeder.play()
                        }
                    }
                }
            }

            // Loading indicator
            BusyIndicator {
                id: progressSpinner
                size: BusyIndicatorSize.Large
                anchors.centerIn: parent
                running: (thumbnailFetcher.fetching ||
                          previewPlayer.status === MediaPlayer.Loading)
            }
        }

        // Keep file details centered
        Column {
            //spacing: Theme.paddingSmall
            anchors.top: fileImage.bottom
            anchors.topMargin: 64
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width

            DetailItem {
                label: qsTr("File name:")
                value: entry.name
            }
            DetailItem {
                label: qsTr("Size:")
                value: fileDetailsHelper.getHRSize(entry.size)
            }
            DetailItem {
                label: qsTr("Last modified:")
                value: Qt.formatDateTime(entry.lastModified, Qt.SystemLocaleShortDate);
                visible: value.length > 0
            }
            DetailItem {
                label: qsTr("Type:")
                value: entry.mimeType
                visible: value.length > 0
            }
            DetailItem {
                label: qsTr("Created at:")
                value: entry.createdAt
                visible: value.length > 0
            }
            DetailItem {
                label: qsTr("Entity tag:")
                value: entry.entityTag
                visible: value.length > 0
            }
        }
    }
}
