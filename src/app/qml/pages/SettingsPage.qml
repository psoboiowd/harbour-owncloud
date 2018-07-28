import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: pageRoot

    Component.onCompleted: {
        persistentSettings.readSettings();
    }

    onStatusChanged: {
        if (status === PageStatus.Deactivating) {
            if (_navigation === PageNavigation.Back) {
                persistentSettings.writeSettings()
                daemonCtrl.reloadConfig()
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        PageHeader {
            id: pageHeader
            title: qsTr("Settings")
        }

        PullDownMenu {
            id: pulley
            MenuItem {
                text: qsTr("Reset connection settings")
                onClicked: {
                    persistentSettings.resetSettings();
                    pageStack.clear();
                    pageStack.push(authenticationEntranceComponent);
                }
            }
        }

        TextSwitch {
            id: autoLoginSwitch
            anchors.top: pageHeader.bottom
            text: qsTr("Login automatically")
            description: qsTr("Automatically log in to your ownCloud server when starting the app", "Login automatically description")
            checked: persistentSettings.autoLogin
            onClicked: persistentSettings.autoLogin = checked
        }

        TextSwitch {
            id: notificationSwitch
            anchors.top: autoLoginSwitch.bottom
            text: qsTr("Notifications")
            description: qsTr("Show global notifications when transfering files", "Notifications description")
            checked: persistentSettings.notifications
            onClicked: persistentSettings.notifications = checked
        }

        TextSwitch {
            id: cameraUploadSwitch
            anchors.top: notificationSwitch.bottom
            text: qsTr("Camera photo backups")
            description: qsTr("Automatically save camera photos to your ownCloud instance when on WiFi", "Camera photo backups escription")
            visible: daemonCtrl.daemonInstalled
            checked: persistentSettings.uploadAutomatically
            onClicked: persistentSettings.uploadAutomatically = checked
        }

        TextSwitch {
            id: mobileCameraUploadSwitch
            anchors.top: cameraUploadSwitch.bottom
            text: qsTr("Photo backups via mobile internet connection")
            description: qsTr("Also automatically backup camera photos when connected via 2G, 3G or LTE", "hoto backups via mobile internet connection description")
            visible: daemonCtrl.daemonInstalled
            enabled: cameraUploadSwitch.checked
            checked: persistentSettings.mobileUpload
            onClicked: persistentSettings.mobileUpload = checked
        }
    }
}
