import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root

    property var plantModel
    property var selectedIds: []

    signal logRequested(int plantId, string entryType, string entryDate, string notes)

    function pad2(value) {
        return value < 10 ? "0" + value : "" + value
    }

    function setSelected(id, isChecked) {
        const idx = root.selectedIds.indexOf(id)
        if (isChecked && idx === -1) {
            root.selectedIds = root.selectedIds.concat([id])
        } else if (!isChecked && idx !== -1) {
            const copy = root.selectedIds.slice()
            copy.splice(idx, 1)
            root.selectedIds = copy
        }
    }

    function clearSelection() {
        root.selectedIds = []
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Batch Journal"
            font.pixelSize: 18
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label { text: "Action" }
            ComboBox {
                id: actionTypeField
                model: ["Water", "Fertilize"]
                editable: false
                currentIndex: 0
            }

            Label { text: "Date" }
            RowLayout {
                spacing: 6

                TextField {
                    id: dateField
                    text: Qt.formatDateTime(new Date(), "yyyy-MM-dd")
                    placeholderText: "YYYY-MM-DD"
                }

                Button {
                    text: "Pick"
                    onClicked: datePopup.open()
                }
            }
        }

        TextArea {
            id: batchNotesField
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            placeholderText: "Notes for all selected plants"
            wrapMode: TextEdit.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: root.selectedIds.length + " selected"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Clear Selection"
                enabled: root.selectedIds.length > 0
                onClicked: root.clearSelection()
            }

            Button {
                text: "Log Entries"
                enabled: root.selectedIds.length > 0
                onClicked: {
                    const dateIso = dateField.text
                    for (let i = 0; i < root.selectedIds.length; i++) {
                        root.logRequested(root.selectedIds[i],
                                          actionTypeField.currentText,
                                          dateIso,
                                          batchNotesField.text)
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.plantModel
            delegate: CheckDelegate {
                width: ListView.view.width
                text: model.name
                checked: root.selectedIds.indexOf(model.id) >= 0
                onToggled: root.setSelected(model.id, checked)
            }
        }

        Popup {
            id: datePopup
            modal: true
            focus: true

            background: Frame { }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label { text: "Pick a date" }

                RowLayout {
                    spacing: 8
                    Label { text: "Year" }
                    SpinBox { id: yearSpin; from: 1970; to: 2100 }
                    Label { text: "Month" }
                    SpinBox { id: monthSpin; from: 1; to: 12 }
                    Label { text: "Day" }
                    SpinBox { id: daySpin; from: 1; to: 31 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Cancel"
                        onClicked: datePopup.close()
                    }
                    Button {
                        text: "Apply"
                        onClicked: {
                            dateField.text = yearSpin.value + "-" + root.pad2(monthSpin.value) + "-" + root.pad2(daySpin.value)
                            datePopup.close()
                        }
                    }
                }
            }

            onOpened: {
                const today = new Date()
                yearSpin.value = today.getFullYear()
                monthSpin.value = today.getMonth() + 1
                daySpin.value = today.getDate()
            }
        }
    }
}
