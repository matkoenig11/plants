import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Frame {
    id: root
    AppConstants { id: ui }

    property var reminderModel
    property var plantModel
    property var settingsModel

    property int selectedReminderId: -1
    property var selectedPlantIds: []

    function setSelectedPlant(id, isChecked) {
        const idx = root.selectedPlantIds.indexOf(id)
        if (isChecked && idx === -1) {
            root.selectedPlantIds = root.selectedPlantIds.concat([id])
        } else if (!isChecked && idx !== -1) {
            const copy = root.selectedPlantIds.slice()
            copy.splice(idx, 1)
            root.selectedPlantIds = copy
        }
    }

    function clearForm() {
        selectedReminderId = -1
        customNameField.text = ""
        taskTypeField.editText = "Water"
        scheduleTypeField.currentIndex = 0
        intervalDaysField.text = "7"
        startDateField.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd")
        nextDueDateField.text = ""
        notesField.text = ""
        root.selectedPlantIds = []
    }

    function buildData() {
        return {
            customName: customNameField.text,
            taskType: taskTypeField.editText,
            scheduleType: scheduleTypeField.currentText,
            intervalDays: parseInt(intervalDaysField.text),
            startDate: startDateField.text,
            nextDueDate: nextDueDateField.text,
            notes: notesField.text
        }
    }

    function computePreviewDate() {
        if (scheduleTypeField.currentText === "interval") {
            const base = Date.fromLocaleString(Qt.locale(), startDateField.text, "yyyy-MM-dd")
            if (base instanceof Date && !isNaN(base)) {
                const days = parseInt(intervalDaysField.text)
                if (days > 0) {
                    const next = new Date(base.getTime())
                    next.setDate(next.getDate() + days)
                    return Qt.formatDateTime(next, "yyyy-MM-dd")
                }
            }
            return ""
        }
        return nextDueDateField.text
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_large

        Label {
            text: "Reminders"
            font.pixelSize: ui.point_size_medium
        }

        Label {
            text: reminderModel.lastError
            color: Material.color(Material.Red)
            visible: text.length > 0
            wrapMode: Text.Wrap
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Frame {
                SplitView.preferredWidth: 300
                ColumnLayout {
                    anchors.fill: parent
                    spacing: ui.spacing_medium

                    Label { text: "Existing" }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: reminderModel
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            text: (model.customName && model.customName.length > 0)
                                  ? model.customName + " • " + model.nextDueDate
                                  : model.taskType + " • " + model.nextDueDate
                            onClicked: {
                                root.selectedReminderId = model.id
                                customNameField.text = model.customName
                                taskTypeField.editText = model.taskType
                                scheduleTypeField.currentIndex = model.scheduleType === "interval" ? 0 : 1
                                intervalDaysField.text = model.intervalDays.toString()
                                startDateField.text = model.startDate
                                nextDueDateField.text = model.nextDueDate
                                notesField.text = model.notes
                                root.selectedPlantIds = model.plantIds
                            }
                        }
                    }
                }
            }

            Frame {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label { text: "Create / Edit" }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: ui.spacing_medium
                        columnSpacing: ui.spacing_large

                        Label { text: "Custom name" }
                        TextField { id: customNameField; placeholderText: "Morning water" }

                        Label { text: "Task" }
                        ComboBox {
                            id: taskTypeField
                            model: ["Water", "Fertilize", "Prune", "Repot", "Other"]
                            editable: true
                            editText: "Water"
                        }

                        Label { text: "Schedule" }
                        ComboBox {
                            id: scheduleTypeField
                            model: ["interval", "fixed"]
                            currentIndex: 0
                        }

                        Label { text: "Interval (days)" }
                        TextField {
                            id: intervalDaysField
                            text: "7"
                            enabled: scheduleTypeField.currentText === "interval"
                        }

                        Label { text: "Start date" }
                        TextField {
                            id: startDateField
                            text: Qt.formatDateTime(new Date(), "yyyy-MM-dd")
                            enabled: scheduleTypeField.currentText === "interval"
                        }

                        Label { text: "Next due date" }
                        TextField {
                            id: nextDueDateField
                            placeholderText: "YYYY-MM-DD"
                            enabled: scheduleTypeField.currentText === "fixed"
                        }

                        Label { text: "Preview" }
                        Label { text: computePreviewDate() }

                        Label { text: "Notes" }
                        TextArea {
                            id: notesField
                            Layout.preferredHeight: ui.notes_text_area_height
                            Layout.fillWidth: true
                            wrapMode: TextEdit.Wrap
                        }
                    }

                    Label { text: "Applies to" }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: ui.standard_panel_height
                        model: plantModel
                        delegate: CheckDelegate {
                            width: ListView.view.width
                            text: model.name
                            checked: root.selectedPlantIds.indexOf(model.id) >= 0
                            onToggled: root.setSelectedPlant(model.id, checked)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ui.spacing_medium

                        Button {
                            text: "Add"
                            onClicked: {
                                const id = reminderModel.addReminder(buildData(), root.selectedPlantIds)
                                if (id > 0) {
                                    clearForm()
                                }
                            }
                        }

                        Button {
                            text: "Update"
                            enabled: root.selectedReminderId > 0
                            onClicked: reminderModel.updateReminder(root.selectedReminderId, buildData(), root.selectedPlantIds)
                        }

                        Button {
                            text: "Delete"
                            enabled: root.selectedReminderId > 0
                            onClicked: {
                                reminderModel.removeReminder(root.selectedReminderId)
                                clearForm()
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Reset"
                            onClicked: clearForm()
                        }
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: ui.spacing_medium

                Label { text: "Delivery Settings" }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    rowSpacing: ui.spacing_medium
                    columnSpacing: ui.spacing_large

                    Label { text: "Day before" }
                    Switch { checked: settingsModel.dayBeforeEnabled; onToggled: settingsModel.dayBeforeEnabled = checked }
                    Label { text: "Time" }
                    TextField { text: settingsModel.dayBeforeTime; onEditingFinished: settingsModel.dayBeforeTime = text }

                    Label { text: "Day of" }
                    Switch { checked: settingsModel.dayOfEnabled; onToggled: settingsModel.dayOfEnabled = checked }
                    Label { text: "Time" }
                    TextField { text: settingsModel.dayOfTime; onEditingFinished: settingsModel.dayOfTime = text }

                    Label { text: "Overdue" }
                    Switch { checked: settingsModel.overdueEnabled; onToggled: settingsModel.overdueEnabled = checked }
                    Label { text: "Cadence (days)" }
                    SpinBox { value: settingsModel.overdueCadenceDays; from: 1; to: 30; onValueChanged: settingsModel.overdueCadenceDays = value }

                    Label { text: "Overdue time" }
                    TextField { text: settingsModel.overdueTime; onEditingFinished: settingsModel.overdueTime = text }
                    Label { text: "Quiet hours" }
                    RowLayout {
                        spacing: ui.spacing_small
                        TextField { text: settingsModel.quietHoursStart; placeholderText: "22:00"; onEditingFinished: settingsModel.quietHoursStart = text }
                        TextField { text: settingsModel.quietHoursEnd; placeholderText: "07:00"; onEditingFinished: settingsModel.quietHoursEnd = text }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ui.spacing_medium

                    Label {
                        text: settingsModel.lastError
                        color: Material.color(Material.Red)
                        visible: text.length > 0
                    }

                    Item { Layout.fillWidth: true }

                    Button { text: "Reload"; onClicked: settingsModel.reload() }
                    Button { text: "Save"; onClicked: settingsModel.save() }
                }
            }
        }
    }
}
