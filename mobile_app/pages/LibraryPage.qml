import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var stackView: null

    function syncProviderIndex() {
        const options = plantLibraryViewModel.providerOptions
        for (let i = 0; i < options.length; ++i) {
            if (options[i].providerId === plantLibraryViewModel.provider) {
                providerCombo.currentIndex = i
                return
            }
        }
        providerCombo.currentIndex = 0
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("Back")
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.pop()
                    }
                }
            }

            Label {
                text: qsTr("Library")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: ui.point_size_medium
            }

            Item { Layout.preferredWidth: implicitWidth }
        }
    }

    background: Rectangle {
        color: "#10171d"
    }

    Component.onCompleted: {
        syncProviderIndex()
        tokenField.text = plantLibraryViewModel.token
    }

    Connections {
        target: plantLibraryViewModel

        function onProviderChanged() {
            root.syncProviderIndex()
            tokenField.text = plantLibraryViewModel.token
        }

        function onTokenChanged() {
            tokenField.text = plantLibraryViewModel.token
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ui.margin_large
        spacing: ui.spacing_medium

        ComboBox {
            id: providerCombo
            Layout.fillWidth: true
            textRole: "displayName"
            model: plantLibraryViewModel.providerOptions
            onActivated: {
                const entry = model[index]
                if (entry && entry.providerId) {
                    plantLibraryViewModel.provider = entry.providerId
                }
            }
        }

        TextField {
            id: tokenField
            Layout.fillWidth: true
            placeholderText: plantLibraryViewModel.tokenLabel
            echoMode: TextInput.Password
            onTextEdited: plantLibraryViewModel.token = text
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ui.spacing_medium

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search Perenual plants")
                onAccepted: plantLibraryViewModel.search(text)
            }

            Button {
                text: qsTr("Search")
                enabled: !plantLibraryViewModel.busy
                onClicked: plantLibraryViewModel.search(searchField.text)
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: plantLibraryViewModel.busy
            visible: running
        }

        Label {
            visible: plantLibraryViewModel.lastError.length > 0
            text: plantLibraryViewModel.lastError
            color: "#ff7b7b"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 14
            color: "#18222c"
            border.color: "#2b3946"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ui.margin_medium
                spacing: ui.spacing_medium

                    Label {
                    text: qsTr("Results")
                    font.pixelSize: ui.point_size_medium
                }

                ListView {
                    id: resultsView
                    Layout.fillWidth: true
                    Layout.preferredHeight: parent.height * 0.38
                    clip: true
                    model: plantLibraryViewModel.results
                    spacing: ui.spacing_small

                    delegate: ItemDelegate {
                        width: resultsView.width
                        onClicked: plantLibraryViewModel.selectResult(index)

                        contentItem: Column {
                            width: parent.width
                            spacing: 2

                            Label {
                                text: modelData.commonName && modelData.commonName.length > 0
                                      ? modelData.commonName
                                      : modelData.scientificName
                                font.pixelSize: ui.point_size_medium
                            }

                            Label {
                                text: modelData.scientificName
                                color: "#93a5b5"
                                font.pixelSize: ui.point_size_small
                            }

                            Label {
                                text: modelData.family
                                color: "#93a5b5"
                                font.pixelSize: ui.point_size_small
                            }
                        }
                    }
                }

                Label {
                    text: qsTr("Details")
                    font.pixelSize: ui.point_size_medium
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        text: plantLibraryViewModel.detailText.length > 0
                              ? plantLibraryViewModel.detailText
                              : qsTr("Search for a plant, then tap a result to view details.")
                    }
                }
            }
        }
    }
}
