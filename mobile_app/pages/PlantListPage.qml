import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var stackView: null
    property var selectedBatchIds: []
    property string batchEntryType: "Water"

    signal plantSelected(var plantData, int index)

    function setSelected(id, checked) {
        const updated = selectedBatchIds.slice();
        const idx = updated.indexOf(id);
        if (checked && idx === -1) {
            updated.push(id);
        } else if (!checked && idx !== -1) {
            updated.splice(idx, 1);
        }
        selectedBatchIds = updated;
    }

    function clearSelection() {
        selectedBatchIds = [];
    }

    function openBatchPopup(entryType) {
        batchEntryType = entryType;
        clearSelection();
        batchEntryPopup.open();
    }

    function applyBatchEntry() {
        if (typeof journalEntryViewModel === "undefined") {
            console.log("journalEntryViewModel not available");
            return;
        }
        const today = Qt.formatDate(new Date(), "yyyy-MM-dd");
        selectedBatchIds.forEach(function(pid) {
            journalEntryViewModel.addEntry(pid, batchEntryType, today, "");
        });
        clearSelection();
        batchEntryPopup.close();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_medium
        anchors.margins: ui.margin_large

        RowLayout {
            Layout.fillWidth: true
            spacing: ui.spacing_medium
            Layout.rightMargin: ui.margin_large
            Layout.leftMargin: ui.margin_large
            Label { text: qsTr("Plants"); font.pixelSize: ui.point_size_large; Layout.fillWidth: true; }
            // Button { text: qsTr("Refresh"); onClicked: plantListViewModel.refresh() }
            Button {
                text: qsTr("Fertilize")
                onClicked: openBatchPopup("Fertilize")
            }
            Button {
                text: qsTr("Batch Water")
                onClicked: openBatchPopup("Water")
            }
        }

        TextField {
            Layout.fillWidth: true
            Layout.leftMargin: ui.margin_large
            Layout.rightMargin: ui.margin_large
            placeholderText: qsTr("Filter by tags, e.g. indoor or herbs, flowers")
            text: plantListViewModel.tagFilter
            onTextEdited: plantListViewModel.tagFilter = text
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: plantListViewModel
            delegate: ItemDelegate {
                width: ListView.view.width
                required property int id
                required property string name
                required property string plantType
                required property string lightRequirement
                required property string wateringFrequency
                required property string notes
                required property string tags
                required property string imageSource
                required property string thumbnailSource

                contentItem: RowLayout {
                    spacing: ui.spacing_medium
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: name
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Label {
                            visible: tags.length > 0
                            text: tags
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            opacity: 0.75
                            font.pixelSize: Math.max(11, ui.point_size_small - 1)
                        }
                    }
                    Image {
                        readonly property string thumbnail: thumbnailSource.length > 0 ? thumbnailSource : imageSource
                        source: thumbnail
                        visible: thumbnail.length > 0
                        Layout.preferredWidth: ui.thumbnail_size_medium
                        Layout.preferredHeight: ui.thumbnail_size_medium
                        fillMode: Image.PreserveAspectCrop
                        verticalAlignment: Image.AlignVCenter
                        clip: true
                    }
                }

                onClicked: {
                    var idx = ListView.view ? ListView.view.indexAt(width / 2, y + height / 2) : -1
                    root.plantSelected({
                        id: id,
                        name: name,
                        plantType: plantType,
                        lightRequirement: lightRequirement,
                        wateringFrequency: wateringFrequency,
                        tags: tags
                    }, idx)
                }
            }
            clip: true
            ScrollBar.vertical: ScrollBar { }
        }
    }

    RoundButton {
        id: addPlantButton
        text: "+"
        font.pixelSize: ui.point_size_xlarge
        width: 60
        height: 60
        radius: width / 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: ui.margin_large
        anchors.bottomMargin: ui.margin_large
        highlighted: true
        z: 10
        onClicked: {
            const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
            if (targetStack) {
                targetStack.push("qrc:/mobile_app/pages/AddPlantLibraryPage.qml", {
                    stackView: targetStack
                })
            }
        }
    }

    Popup {
        id: batchEntryPopup
        modal: true
        focus: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, ui.popup_width)
        height: Math.min(parent.height * 0.75, ui.popup_height)

        ColumnLayout {
            anchors.fill: parent
            spacing: ui.spacing_medium

            Label {
                text: batchEntryType === "Fertilize"
                      ? qsTr("Select plants to mark fertilized")
                      : qsTr("Select plants to mark watered")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: plantListViewModel
                    delegate: RowLayout {
                        width: ListView.view ? ListView.view.width : undefined
                        spacing: ui.spacing_medium
                        CheckBox {
                            checked: selectedBatchIds.indexOf(id) !== -1
                            onToggled: root.setSelected(id, checked)
                        }
                        Label {
                            text: name
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ui.spacing_medium
                Button {
                    text: qsTr("Cancel")
                    onClicked: batchEntryPopup.close()
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: batchEntryType === "Fertilize" ? qsTr("Fertilize") : qsTr("Water")
                    enabled: selectedBatchIds.length > 0
                    onClicked: applyBatchEntry()
                }
            }
        }
    }
}
