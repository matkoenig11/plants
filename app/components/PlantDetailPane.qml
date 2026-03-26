import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Frame {
    id: root
    AppConstants { id: ui }

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
            lightRequirement: lightRequirementField.text,
            indoor: indoorField.text,
            floweringSeason: floweringSeasonField.text,
            wateringFrequency: wateringFrequencyField.text,
            wateringNotes: wateringNotesField.text,
            humidityPreference: humidityPreferenceField.text,
            soilType: soilTypeField.text,
            lastWatered: lastWateredField.text,
            fertilizingSchedule: fertilizingScheduleField.text,
            lastFertilized: lastFertilizedField.text,
            pruningTime: pruningTimeField.text,
            pruningNotes: pruningNotesField.text,
            lastPruned: lastPrunedField.text,
            growthRate: growthRateField.text,
            issuesPests: issuesPestsField.text,
            temperatureTolerance: temperatureToleranceField.text,
            toxicToPets: toxicToPetsField.text,
            poisonousToHumans: poisonousToHumansField.text,
            poisonousToPets: poisonousToPetsField.text,
            acquiredDate: acquiredDateField.text,
            source: sourceField.text,
            notes: notesField.text
        }
    }

    function clearFields() {
        nameField.text = ""
        scientificNameField.text = ""
        plantTypeField.text = ""
        lightRequirementField.text = ""
        indoorField.text = ""
        floweringSeasonField.text = ""
        wateringFrequencyField.text = ""
        wateringNotesField.text = ""
        humidityPreferenceField.text = ""
        soilTypeField.text = ""
        lastWateredField.text = ""
        fertilizingScheduleField.text = ""
        lastFertilizedField.text = ""
        pruningTimeField.text = ""
        pruningNotesField.text = ""
        lastPrunedField.text = ""
        growthRateField.text = ""
        issuesPestsField.text = ""
        temperatureToleranceField.text = ""
        toxicToPetsField.text = ""
        poisonousToHumansField.text = ""
        poisonousToPetsField.text = ""
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
        lightRequirementField.text = plantData.lightRequirement || ""
        indoorField.text = plantData.indoor || ""
        floweringSeasonField.text = plantData.floweringSeason || ""
        wateringFrequencyField.text = plantData.wateringFrequency || ""
        wateringNotesField.text = plantData.wateringNotes || ""
        humidityPreferenceField.text = plantData.humidityPreference || ""
        soilTypeField.text = plantData.soilType || ""
        lastWateredField.text = plantData.lastWatered || ""
        fertilizingScheduleField.text = plantData.fertilizingSchedule || ""
        lastFertilizedField.text = plantData.lastFertilized || ""
        pruningTimeField.text = plantData.pruningTime || ""
        pruningNotesField.text = plantData.pruningNotes || ""
        lastPrunedField.text = plantData.lastPruned || ""
        growthRateField.text = plantData.growthRate || ""
        issuesPestsField.text = plantData.issuesPests || ""
        temperatureToleranceField.text = plantData.temperatureTolerance || ""
        toxicToPetsField.text = plantData.toxicToPets || ""
        poisonousToHumansField.text = plantData.poisonousToHumans || ""
        poisonousToPetsField.text = plantData.poisonousToPets || ""
        acquiredDateField.text = plantData.acquiredDate || ""
        sourceField.text = plantData.source || ""
        notesField.text = plantData.notes || ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_large

        Label {
            text: "Details"
            font.pixelSize: ui.point_size_large
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
            contentWidth: formLayout.width

            ColumnLayout {
                id: formLayout
                width: Math.min(parent.width * 0.66, parent.width)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: ui.spacing_large

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: ui.spacing_medium
                    columnSpacing: ui.spacing_large

                    Label { text: "Name" }
                    TextField { id: nameField; placeholderText: "Calathea" }

                    Label { text: "Scientific Name" }
                    TextField { id: scientificNameField; placeholderText: "Calathea (unknown sp.)" }

                    Label { text: "Plant Type" }
                    TextField { id: plantTypeField; placeholderText: "Houseplant" }

                    Label { text: "Light Requirement" }
                    TextField { id: lightRequirementField; placeholderText: "Bright indirect" }

                    Label { text: "Indoor" }
                    TextField { id: indoorField; placeholderText: "Yes/No/Sometimes" }

                    Label { text: "Flowering Season" }
                    TextField { id: floweringSeasonField; placeholderText: "Spring to summer" }

                    Label { text: "Watering Frequency" }
                    TextField { id: wateringFrequencyField; placeholderText: "1-2x/week" }

                    Label { text: "Watering Notes" }
                    TextArea {
                        id: wateringNotesField
                        Layout.preferredHeight: ui.form_text_area_height
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Humidity Preference" }
                    TextField { id: humidityPreferenceField; placeholderText: "High" }

                    Label { text: "Soil Type" }
                    TextField { id: soilTypeField; placeholderText: "" }

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
                        Layout.preferredHeight: ui.form_text_area_height
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Last Pruned" }
                    TextField { id: lastPrunedField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Growth Rate" }
                    TextField { id: growthRateField; placeholderText: "" }

                    Label { text: "Issues / Pests" }
                    TextArea {
                        id: issuesPestsField
                        Layout.preferredHeight: ui.form_text_area_height
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }

                    Label { text: "Temperature Tolerance" }
                    TextField { id: temperatureToleranceField; placeholderText: "" }

                    Label { text: "Toxic to Pets" }
                    TextField { id: toxicToPetsField; placeholderText: "Yes/No/Unknown" }

                    Label { text: "Poisonous to Humans" }
                    TextField { id: poisonousToHumansField; placeholderText: "Yes/No/Unknown" }

                    Label { text: "Poisonous to Pets" }
                    TextField { id: poisonousToPetsField; placeholderText: "Yes/No/Unknown" }

                    Label { text: "Acquired Date" }
                    TextField { id: acquiredDateField; placeholderText: "YYYY-MM-DD" }

                    Label { text: "Source" }
                    TextField { id: sourceField; placeholderText: "" }

                    Label { text: "Personal Notes" }
                    TextArea {
                        id: notesField
                        Layout.preferredHeight: ui.notes_text_area_height
                        Layout.fillWidth: true
                        wrapMode: TextEdit.Wrap
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ui.spacing_medium

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
            spacing: ui.spacing_medium

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
