import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import "components"

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "Plant Journal"
    property bool darkMode: false
    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: darkMode ? "#8BC34A" : "#2E7D32"

    property int selectedPlantId: -1
    property var selectedPlantData: null

    RowLayout {
        anchors.fill: parent
        spacing: 0

        PlantListPane {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            plantModel: plantListViewModel
            onPlantSelected: function(plantData) {
                root.selectedPlantId = plantData.id
                root.selectedPlantData = plantData
                journalEntryViewModel.setPlantId(plantData.id)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label { text: "Theme" }

                Switch {
                    id: themeSwitch
                    text: root.darkMode ? "Dark" : "Light"
                    checked: root.darkMode
                    onToggled: root.darkMode = checked
                }

                Item { Layout.fillWidth: true }
            }

            TabBar {
                id: detailTabs
                Layout.fillWidth: true
                TabButton { text: "Details" }
                TabButton { text: "Journal" }
                TabButton { text: "Batch Log" }
                TabButton { text: "Reminders" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: detailTabs.currentIndex

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        PlantDetailPane {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            plantId: root.selectedPlantId
                            plantData: root.selectedPlantData
                            errorText: plantListViewModel.lastError
                            onAddRequested: function(data) {
                                const id = plantListViewModel.addPlant(data)
                                if (id > 0) {
                                    root.selectedPlantId = -1
                                    root.selectedPlantData = null
                                }
                            }
                            onUpdateRequested: function(id, data) {
                                plantListViewModel.updatePlant(id, data)
                            }
                            onDeleteRequested: function(id) {
                                plantListViewModel.removePlant(id)
                                root.selectedPlantId = -1
                                root.selectedPlantData = null
                                journalEntryViewModel.setPlantId(-1)
                            }
                            onQuickJournalRequested: function(plantId, entryType, entryDate, notes) {
                                journalEntryViewModel.addEntry(plantId, entryType, entryDate, notes)
                                detailTabs.currentIndex = 1
                            }
                            onOpenBatchRequested: function() {
                                detailTabs.currentIndex = 2
                            }
                        }

                        Frame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: "Recent journal entries" }
                                    Item { Layout.fillWidth: true }
                                    Button {
                                        text: "Open Journal"
                                        onClicked: detailTabs.currentIndex = 1
                                    }
                                }

                                Label {
                                    text: root.selectedPlantId > 0 ? "" : "Select a plant to view journal entries."
                                    visible: root.selectedPlantId <= 0
                                    color: Material.hintTextColor
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    model: journalEntryViewModel
                                    visible: root.selectedPlantId > 0
                                    delegate: Label {
                                        text: model.entryType + " • " + model.entryDate
                                        color: Material.secondaryTextColor
                                    }
                                }
                            }
                        }
                    }
                }

                JournalEntryPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    entryModel: journalEntryViewModel
                    plantId: root.selectedPlantId
                    errorText: journalEntryViewModel.lastError
                    onAddRequested: function(plantId, entryType, entryDate, notes) {
                        journalEntryViewModel.addEntry(plantId, entryType, entryDate, notes)
                    }
                    onUpdateRequested: function(id, plantId, entryType, entryDate, notes) {
                        journalEntryViewModel.updateEntry(id, plantId, entryType, entryDate, notes)
                    }
                    onDeleteRequested: function(entryId) {
                        journalEntryViewModel.removeEntry(entryId)
                    }
                }

                BatchJournalPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    plantModel: plantListViewModel
                    onLogRequested: function(plantId, entryType, entryDate, notes) {
                        journalEntryViewModel.addEntry(plantId, entryType, entryDate, notes)
                    }
                }

                ReminderPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    reminderModel: reminderListViewModel
                    plantModel: plantListViewModel
                    settingsModel: reminderSettingsViewModel
                }
            }
        }
    }
}
