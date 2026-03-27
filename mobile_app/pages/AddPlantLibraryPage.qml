import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }

    property var stackView: null
    property var draftPlantData: ({})
    readonly property int cardWidth: Math.min(width - (ui.margin_large * 2), 760)
    readonly property int cardPadding: ui.margin_large

    readonly property color pageBackgroundColor: "#10171d"
    readonly property color cardBackgroundColor: "#18222c"
    readonly property color cardBorderColor: "#2b3946"
    readonly property color secondaryTextColor: "#93a5b5"

    function copyMap(source) {
        const copy = {}
        if (!source) {
            return copy
        }
        for (let key in source) {
            copy[key] = source[key]
        }
        return copy
    }

    function targetStack() {
        return root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
    }

    function openManualEditor(initialData) {
        const stack = targetStack()
        if (!stack) {
            return
        }
        stack.push("qrc:/mobile_app/pages/PlantDetailEditPage.qml", {
            plantData: initialData || ({}),
            stackView: stack
        })
    }

    function applySelectedPlantDraft() {
        draftPlantData = copyMap(plantLibraryViewModel.selectedPlantData)
    }

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

    Component.onCompleted: {
        applySelectedPlantDraft()
        syncProviderIndex()
        tokenField.text = plantLibraryViewModel.token
    }

    Connections {
        target: plantLibraryViewModel

        function onSelectedPlantDataChanged() {
            root.applySelectedPlantDraft()
        }

        function onProviderChanged() {
            root.syncProviderIndex()
            tokenField.text = plantLibraryViewModel.token
        }

        function onTokenChanged() {
            tokenField.text = plantLibraryViewModel.token
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("Back")
                onClicked: {
                    const stack = root.targetStack()
                    if (stack) {
                        stack.pop()
                    }
                }
            }

            Label {
                text: qsTr("Add Plant")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: ui.point_size_medium
            }

            ToolButton {
                text: qsTr("Manual")
                onClicked: root.openManualEditor({})
            }
        }
    }

    background: Rectangle {
        color: root.pageBackgroundColor
    }

    ScrollView {
        id: pageScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Column {
            width: pageScroll.availableWidth
            spacing: ui.spacing_large

            Item {
                width: 1
                height: ui.margin_large
            }

            Rectangle {
                width: root.cardWidth
                height: implicitHeight
                implicitHeight: searchCardLayout.implicitHeight + (root.cardPadding * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 18
                color: root.cardBackgroundColor
                border.color: root.cardBorderColor

                ColumnLayout {
                    id: searchCardLayout
                    x: root.cardPadding
                    y: root.cardPadding
                    width: parent.width - (root.cardPadding * 2)
                    spacing: ui.spacing_medium

                    Label {
                        text: qsTr("Search Plant Library")
                        font.pixelSize: ui.point_size_large
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Search first, then continue into the normal plant editor with the Perenual data prefilled.")
                        wrapMode: Text.Wrap
                        color: root.secondaryTextColor
                        Layout.fillWidth: true
                    }

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
                            placeholderText: qsTr("Search plants")
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
                }
            }

            Rectangle {
                width: root.cardWidth
                height: implicitHeight
                implicitHeight: resultsCardLayout.implicitHeight + (root.cardPadding * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 18
                color: root.cardBackgroundColor
                border.color: root.cardBorderColor

                ColumnLayout {
                    id: resultsCardLayout
                    x: root.cardPadding
                    y: root.cardPadding
                    width: parent.width - (root.cardPadding * 2)
                    spacing: ui.spacing_medium

                    Label {
                        text: qsTr("Results")
                        font.pixelSize: ui.point_size_medium
                        font.bold: true
                    }

                    ListView {
                        id: resultsView
                        Layout.fillWidth: true
                        Layout.preferredHeight: 240
                        clip: true
                        spacing: ui.spacing_small
                        model: plantLibraryViewModel.results

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
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: modelData.scientificName
                                    color: root.secondaryTextColor
                                    font.pixelSize: ui.point_size_small
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: modelData.family
                                    color: root.secondaryTextColor
                                    font.pixelSize: ui.point_size_small
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar { }
                    }
                }
            }

            Rectangle {
                width: root.cardWidth
                height: implicitHeight
                implicitHeight: draftCardLayout.implicitHeight + (root.cardPadding * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 18
                color: root.cardBackgroundColor
                border.color: root.cardBorderColor

                ColumnLayout {
                    id: draftCardLayout
                    x: root.cardPadding
                    y: root.cardPadding
                    width: parent.width - (root.cardPadding * 2)
                    spacing: ui.spacing_medium

                    Label {
                        text: qsTr("Mapped Plant Draft")
                        font.pixelSize: ui.point_size_medium
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: ui.spacing_medium
                        rowSpacing: ui.spacing_small

                        Label { text: qsTr("Name"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.name || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Scientific name"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.scientificName || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Plant type"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.plantType || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Light"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.lightRequirement || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Watering"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.wateringFrequency || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Soil"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.soilType || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Indoor"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.indoor || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }

                        Label { text: qsTr("Flowering"); color: root.secondaryTextColor }
                        Label { text: draftPlantData.floweringSeason || "-" ; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }

                    Button {
                        text: qsTr("Use Selected Plant")
                        Layout.fillWidth: true
                        enabled: (draftPlantData.name || "").length > 0
                        onClicked: root.openManualEditor(root.copyMap(draftPlantData))
                    }
                }
            }

            Rectangle {
                width: root.cardWidth
                height: implicitHeight
                implicitHeight: detailsCardLayout.implicitHeight + (root.cardPadding * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 18
                color: root.cardBackgroundColor
                border.color: root.cardBorderColor

                ColumnLayout {
                    id: detailsCardLayout
                    x: root.cardPadding
                    y: root.cardPadding
                    width: parent.width - (root.cardPadding * 2)
                    spacing: ui.spacing_medium

                    Label {
                        text: qsTr("Details")
                        font.pixelSize: ui.point_size_medium
                        font.bold: true
                    }

                    TextArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 280
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        text: plantLibraryViewModel.detailText.length > 0
                              ? plantLibraryViewModel.detailText
                              : qsTr("Search for a plant, then tap a result to inspect the details before continuing.")
                    }
                }
            }

            Item {
                width: 1
                height: ui.margin_large * 2
            }
        }
    }
}
