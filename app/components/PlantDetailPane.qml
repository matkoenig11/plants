import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root

    property int plantId: -1
    property var plantData
    property string errorText: ""

    signal addRequested(var data)
    signal updateRequested(int id, var data)
    signal deleteRequested(int id)
    signal quickJournalRequested(int plantId, string entryType, string entryDate, string notes)
    signal openBatchRequested()

    function collectData() {
        return {
            name: nameField.text,
            scientificName: scientificNameField.text,
            plantType: plantTypeField.text,
            winterLocation: winterLocationField.text,
            summerLocation: summerLocationField.text,
            lightRequirement: lightRequirementField.text,
            wateringFrequency: wateringFrequencyField.text,
            wateringNotes: wateringNotesField.text,
            humidityPreference: humidityPreferenceField.text,
            soilType: soilTypeField.text,
            potSize: potSizeField.text,
            lastWatered: lastWateredField.text,
            fertilizingSchedule: fertilizingScheduleField.text,
            lastFertilized: lastFertilizedField.text,
            pruningTime: pruningTimeField.text,
            pruningNotes: pruningNotesField.text,
            lastPruned: lastPrunedField.text,
            growthRate: growthRateField.text,
            currentHealthStatus: currentHealthStatusField.text,
            issuesPests: issuesPestsField.text,
            temperatureTolerance: temperatureToleranceField.text,
            toxicToPets: toxicToPetsField.text,
            acquiredDate: acquiredDateField.text,
            source: sourceField.text,
            notes: notesField.text
        }
    }

    function clearFields() {
        nameField.text = ""
        scientificNameField.text = ""
        plantTypeField.text = ""
        winterLocationField.text = ""
        summerLocationField.text = ""
        lightRequirementField.text = ""
        wateringFrequencyField.text = ""
        wateringNotesField.text = ""
        humidityPreferenceField.text = ""
        soilTypeField.text = ""
        potSizeField.text = ""
        lastWateredField.text = ""
        fertilizingScheduleField.text = ""
        lastFertilizedField.text = ""
        pruningTimeField.text = ""
        pruningNotesField.text = ""
        lastPrunedField.text = ""
        growthRateField.text = ""
        currentHealthStatusField.text = ""
        issuesPestsField.text = ""
        temperatureToleranceField.text = ""
        toxicToPetsField.text = ""
        acquiredDateField.text = ""
        sourceField.text = ""
        notesField.text = ""
    }

    onPlantDataChanged: {
        if (!plantData) {
            clearFields()
            return
        }
        nameField.text = plantData.name || ""
        scientificNameField.text = plantData.scientificName || ""
        plantTypeField.text = plantData.plantType || ""
        winterLocationField.text = plantData.winterLocation || ""
        summerLocationField.text = plantData.summerLocation || ""
        lightRequirementField.text = plantData.lightRequirement || ""
        wateringFrequencyField.text = plantData.wateringFrequency || ""
        wateringNotesField.text = plantData.wateringNotes || ""
        humidityPreferenceField.text = plantData.humidityPreference || ""
        soilTypeField.text = plantData.soilType || ""
        potSizeField.text = plantData.potSize || ""
        lastWateredField.text = plantData.lastWatered || ""
        fertilizingScheduleField.text = plantData.fertilizingSchedule || ""
        lastFertilizedField.text = plantData.lastFertilized || ""
        pruningTimeField.text = plantData.pruningTime || ""
        pruningNotesField.text = plantData.pruningNotes || ""
        lastPrunedField.text = plantData.lastPruned || ""
        growthRateField.text = plantData.growthRate || ""
        currentHealthStatusField.text = plantData.currentHealthStatus || ""
        issuesPestsField.text = plantData.issuesPests || ""
        temperatureToleranceField.text = plantData.temperatureTolerance || ""
        toxicToPetsField.text = plantData.toxicToPets || ""
        acquiredDateField.text = plantData.acquiredDate || ""
        sourceField.text = plantData.source || ""
        notesField.text = plantData.notes || ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Details"
            font.pixelSize: 20
        }

        Label {
            text: root.errorText
            color: Material.color(Material.Red)
            visible: text.length > 0
            wrapMode: Text.Wrap
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 12

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 8
                    columnSpacing: 12

                    Label { text: "Name" }
                    TextField { id: nameField; placeholderText: "Calathea" }

                    Label { text: "Scientific Name" }
                    TextField { id: scientificNameField; placeholderText: "Calathea (unknown sp.)" }

                    Label { text: "Plant Type" }
                    TextField { id: plantTypeField; placeholderText: "Houseplant" }

                    Label { text: "Winter Location" }
                    TextField { id: winterLocationField; placeholderText: "Indoors" }

                    Label { text: "Summer Location" }
                    TextField { id: summerLocationField; placeholderText: "" }

                    Label { text: "Light Requirement" }
                    TextField { id: lightRequirementField; placeholderText: "Bright indirect" }

                    Label { text: "Watering Frequency" }
                    TextField { id: wateringFrequencyField; placeholderText: "1-2x/week" }

                    Label { text: "Watering Notes" }
                    TextArea {
                        id: wateringNotesField
                        Layout.preferredHeight: 60
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Humidity Preference" }
                    TextField { id: humidityPreferenceField; placeholderText: "High" }

                    Label { text: "Soil Type" }
                    TextField { id: soilTypeField; placeholderText: "" }

                    Label { text: "Pot Size" }
                    TextField { id: potSizeField; placeholderText: "" }

                    Label { text: "Last Watered" }
                    TextField { id: lastWateredField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Fertilizing Schedule" }
                    TextField { id: fertilizingScheduleField; placeholderText: "" }

                    Label { text: "Last Fertilized" }
                    TextField { id: lastFertilizedField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Pruning Time" }
                    TextField { id: pruningTimeField; placeholderText: "" }

                    Label { text: "Pruning Notes" }
                    TextArea {
                        id: pruningNotesField
                        Layout.preferredHeight: 60
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Last Pruned" }
                    TextField { id: lastPrunedField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Growth Rate" }
                    TextField { id: growthRateField; placeholderText: "" }

                    Label { text: "Current Health Status" }
                    TextField { id: currentHealthStatusField; placeholderText: "" }

                    Label { text: "Issues / Pests" }
                    TextArea {
                        id: issuesPestsField
                        Layout.preferredHeight: 60
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Temperature Tolerance" }
                    TextField { id: temperatureToleranceField; placeholderText: "" }

                    Label { text: "Toxic to Pets" }
                    TextField { id: toxicToPetsField; placeholderText: "Yes/No/Unknown" }

                    Label { text: "Acquired Date" }
                    TextField { id: acquiredDateField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Source" }
                    TextField { id: sourceField; placeholderText: "" }

                    Label { text: "Personal Notes" }
                    TextArea {
                        id: notesField
                        Layout.preferredHeight: 80
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Add"
                onClicked: root.addRequested(root.collectData())
            }

            Button {
                text: "Update"
                enabled: root.plantId > 0
                onClicked: root.updateRequested(root.plantId, root.collectData())
            }

            Button {
                text: "Delete"
                enabled: root.plantId > 0
                onClicked: root.deleteRequested(root.plantId)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Log Watered"
                enabled: root.plantId > 0
                onClicked: root.quickJournalRequested(root.plantId,
                                                     "Water",
                                                     Qt.formatDateTime(new Date(), "yyyy-MM-dd"),
                                                     "")
            }

            Button {
                text: "Log Fertilized"
                enabled: root.plantId > 0
                onClicked: root.quickJournalRequested(root.plantId,
                                                     "Fertilize",
                                                     Qt.formatDateTime(new Date(), "yyyy-MM-dd"),
                                                     "")
            }

            Button {
                text: "Batch Log"
                onClicked: root.openBatchRequested()
            }
        }
    }
}
