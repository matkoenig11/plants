import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import "../../ui"

Page {
    id: root
    property var stackView: null
    AppConstants { id: ui }

    title: qsTr("Sync Server")

    MessageDialog {
        id: forcePullDialog
        title: qsTr("Force Pull")
        text: qsTr("This will delete all local plants, journal entries, reminders, images, schedules, and sync tombstones, then replace them with data from the sync server. Continue?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: databaseConnectionViewModel.forcePull()
    }

    MessageDialog {
        id: clearLocalDatabaseDialog
        title: qsTr("Clear Local Database")
        text: qsTr("This will delete all local plants, journal entries, reminders, images, schedules, and sync tombstones from this device. Sync server settings will be kept, and the next sync will start from scratch. Continue?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: databaseConnectionViewModel.clearLocalDatabase()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: ui.spacing_medium

            ToolButton {
                text: "\u2190"
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.pop()
                    }
                }
            }

            Label {
                text: qsTr("Sync Server")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: ui.spacing_medium

            Frame {
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: ui.spacing_small

                    Label {
                        text: qsTr("HTTP Sync")
                        font.pixelSize: ui.point_size_medium
                        font.bold: true
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Server URL")
                        text: databaseConnectionViewModel.baseUrl
                        onTextChanged: databaseConnectionViewModel.baseUrl = text
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Device token")
                        text: databaseConnectionViewModel.deviceToken
                        echoMode: TextInput.Password
                        onTextChanged: databaseConnectionViewModel.deviceToken = text
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Device label (optional)")
                        text: databaseConnectionViewModel.deviceLabel
                        onTextChanged: databaseConnectionViewModel.deviceLabel = text
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Use Synchronize after saving to push local SQLite changes to the sync service and pull newer server changes back.")
                        color: "#d9d9d9"
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("The sync service should be reachable over your VPN and configured with a long-lived device token.")
                        color: "#b0b0b0"
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Local database: %1").arg(databaseConnectionViewModel.localDatabasePath)
                        color: "#b0b0b0"
                    }
                }
            }

            Frame {
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: ui.spacing_small

                    Label {
                        visible: databaseConnectionViewModel.lastError.length > 0
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: "#ff6b6b"
                        text: databaseConnectionViewModel.lastError
                    }

                    Label {
                        visible: databaseConnectionViewModel.lastSyncSummary.length > 0
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: "#d9f0c1"
                        text: databaseConnectionViewModel.lastSyncSummary
                    }

                    Label {
                        visible: databaseConnectionViewModel.connectionStatus.length > 0
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: databaseConnectionViewModel.connectionOk ? "#8ee28e" : "#ffd166"
                        text: (databaseConnectionViewModel.connectionOk ? "\u2713 " : "\u26a0 ")
                              + databaseConnectionViewModel.connectionStatus
                    }
                    ColumnLayout {
                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                text: qsTr("Reload")
                                onClicked: databaseConnectionViewModel.reload()
                            }

                            Button {
                                text: qsTr("Save")
                                onClicked: databaseConnectionViewModel.save()
                            }

                            Button {
                                text: qsTr("Test Connection")
                                onClicked: databaseConnectionViewModel.testConnection()
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                text: qsTr("Synchronize")
                                highlighted: true
                                onClicked: databaseConnectionViewModel.synchronize()
                            }
                            Button {
                                text: qsTr("Force Pull")
                                onClicked: forcePullDialog.open()
                            }

                            Button {
                                text: qsTr("Clear Local DB")
                                onClicked: clearLocalDatabaseDialog.open()
                            }
                        }
                    }
                }
            }
        }
    }
}
