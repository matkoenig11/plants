import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root

    property var entryModel
    property int plantId: -1
    property string errorText: ""
    property int selectedEntryId: -1

    signal addRequested(int plantId, string entryType, string entryDate, string notes)
    signal updateRequested(int id, int plantId, string entryType, string entryDate, string notes)
    signal deleteRequested(int entryId)

    onPlantIdChanged: {
        root.selectedEntryId = -1
        typeField.editText = "Water"
        dateField.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd")
        notesField.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Journal"
            font.pixelSize: 18
        }

        Label {
            text: root.errorText
            color: Material.color(Material.Red)
            visible: text.length > 0
            wrapMode: Text.Wrap
        }

        ListView {
            id: entryList
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            model: root.entryModel
            delegate: ItemDelegate {
                width: ListView.view.width
                padding: 8
                onClicked: {
                    root.selectedEntryId = model.id
                    typeField.editText = model.entryType
                    dateField.text = model.entryDate
                    notesField.text = model.notes
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: model.entryType + " - " + model.entryDate }
                        Label { text: model.notes; color: Material.secondaryTextColor; wrapMode: Text.Wrap }
                    }

                    Button {
                        text: "Delete"
                        onClicked: root.deleteRequested(model.id)
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 8
            columnSpacing: 12

            Label { text: "Type" }
            ComboBox {
                id: typeField
                model: ["Water", "Fertilize", "Prune", "Repot", "Other"]
                editable: true
                editText: "Water"
            }

            Label { text: "Date" }
            TextField { id: dateField; placeholderText: "YYYY-MM-DD" }

            Label { text: "Notes" }
            TextArea {
                id: notesField
                Layout.preferredHeight: 80
                Layout.fillWidth: true
                wrapMode: TextEdit.Wrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Add Entry"
                enabled: root.plantId > 0
                onClicked: {
                    root.addRequested(root.plantId,
                                      typeField.editText,
                                      dateField.text,
                                      notesField.text)
                    if (root.plantId > 0) {
                        root.selectedEntryId = -1
                        typeField.editText = "Water"
                        dateField.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd")
                        notesField.text = ""
                    }
                }
            }

            Button {
                text: "Update Entry"
                enabled: root.selectedEntryId > 0
                onClicked: root.updateRequested(root.selectedEntryId,
                                                root.plantId,
                                                typeField.editText,
                                                dateField.text,
                                                notesField.text)
            }
        }
    }
}
