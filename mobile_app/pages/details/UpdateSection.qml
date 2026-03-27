import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15
import "../../../ui"
import "../../utils/CareFrequency.js" as CareFrequency

ColumnLayout {
    id: root
    AppConstants { id: ui }

    property var plantData: ({})
    property var scheduleData: ({})
    property string statusText: ""
    property color statusColor: "red"
    property bool schedulesExpanded: false
    readonly property var seasonNames: ["winter", "spring", "summer", "autumn"]
    readonly property var parsedWateringFrequency: CareFrequency.parseWateringFrequency(val("wateringFrequency", ""))

    function val(key, fallback) {
        if (plantData && plantData[key] !== undefined && plantData[key] !== "") {
            return plantData[key];
        }
        return fallback !== undefined ? fallback : "";
    }

    function defaultSeasonMap() {
        return {
            winter: { useOverride: false, enabled: true, intervalDays: 0 },
            spring: { useOverride: false, enabled: true, intervalDays: 0 },
            summer: { useOverride: false, enabled: true, intervalDays: 0 },
            autumn: { useOverride: false, enabled: true, intervalDays: 0 }
        };
    }

    function emptySchedule() {
        return {
            activeSeason: "",
            water: {
                defaultIntervalDays: 0,
                activeEnabled: false,
                activeIntervalDays: 0,
                summary: qsTr("Not set"),
                seasonal: defaultSeasonMap()
            },
            fertilize: {
                defaultIntervalDays: 0,
                activeEnabled: false,
                activeIntervalDays: 0,
                summary: qsTr("Not set"),
                seasonal: defaultSeasonMap()
            }
        };
    }

    function cloneSchedule() {
        return JSON.parse(JSON.stringify(scheduleData));
    }

    function careMap(careKey) {
        const current = scheduleData && scheduleData[careKey];
        if (current)
            return current;
        return emptySchedule()[careKey];
    }

    function seasonMap(careKey, seasonName) {
        const current = careMap(careKey);
        if (current.seasonal && current.seasonal[seasonName])
            return current.seasonal[seasonName];
        return defaultSeasonMap()[seasonName];
    }

    function seasonLabel(seasonName) {
        return seasonName.charAt(0).toUpperCase() + seasonName.slice(1);
    }

    function intervalText(value) {
        return value && value > 0 ? String(value) : "";
    }

    function parseIsoDate(text) {
        if (!text)
            return null;
        const parts = text.split("-");
        if (parts.length !== 3)
            return null;
        const year = parseInt(parts[0], 10);
        const month = parseInt(parts[1], 10);
        const day = parseInt(parts[2], 10);
        if (!year || !month || !day)
            return null;
        const date = new Date(year, month - 1, day);
        return isNaN(date.getTime()) ? null : date;
    }

    function nextCareDateText(lastDateText, careKey) {
        const current = careMap(careKey);
        if (!current.activeEnabled)
            return qsTr("Paused this season");
        if (!current.activeIntervalDays || current.activeIntervalDays <= 0)
            return qsTr("Not scheduled");

        const lastDate = parseIsoDate(lastDateText);
        if (!lastDate)
            return qsTr("Unknown");

        const nextDate = new Date(lastDate.getFullYear(), lastDate.getMonth(), lastDate.getDate());
        nextDate.setDate(nextDate.getDate() + current.activeIntervalDays);
        return Qt.formatDate(nextDate, "yyyy-MM-dd");
    }

    function lastCareText(lastDateText) {
        const lastDate = parseIsoDate(lastDateText);
        if (!lastDate)
            return qsTr("Unknown");

        const today = new Date();
        const startOfToday = new Date(today.getFullYear(), today.getMonth(), today.getDate());
        const startOfLast = new Date(lastDate.getFullYear(), lastDate.getMonth(), lastDate.getDate());
        const diffMs = startOfLast.getTime() - startOfToday.getTime();
        const diffDays = Math.round(diffMs / 86400000);

        if (diffDays === 0)
            return qsTr("Today");
        if (diffDays === -1)
            return qsTr("1 day ago");
        if (diffDays < 0) {
            const daysAgo = -diffDays;
            const weeks = Math.floor(daysAgo / 7);
            const remainingDays = daysAgo % 7;
            if (weeks > 0 && remainingDays > 0) {
                return qsTr("%1 week%2 and %3 day%4 ago")
                    .arg(weeks)
                    .arg(weeks === 1 ? "" : "s")
                    .arg(remainingDays)
                    .arg(remainingDays === 1 ? "" : "s");
            }
            if (weeks > 0) {
                return qsTr("%1 week%2 ago")
                    .arg(weeks)
                    .arg(weeks === 1 ? "" : "s");
            }
            return qsTr("%1 day%2 ago")
                .arg(daysAgo)
                .arg(daysAgo === 1 ? "" : "s");
        }

        if (diffDays === 1)
            return qsTr("Tomorrow");
        return qsTr("In %1 days").arg(diffDays);
    }

    function activeCareText(careKey) {
        const current = careMap(careKey);
        if (!current.activeEnabled)
            return qsTr("Paused this season");
        if (current.activeIntervalDays > 0)
            return qsTr("Every %1 days").arg(current.activeIntervalDays);
        return qsTr("Not set");
    }

    function elapsedDaysSince(dateText) {
        const lastDate = parseIsoDate(dateText);
        if (!lastDate)
            return null;

        const today = new Date();
        const startOfToday = new Date(today.getFullYear(), today.getMonth(), today.getDate());
        const startOfLast = new Date(lastDate.getFullYear(), lastDate.getMonth(), lastDate.getDate());
        return Math.round((startOfToday.getTime() - startOfLast.getTime()) / 86400000);
    }

    function lastWateredStatus() {
        if (!parsedWateringFrequency.valid)
            return "";

        const elapsedDays = elapsedDaysSince(val("lastWatered", ""));
        if (elapsedDays === null)
            return "";

        if (elapsedDays < parsedWateringFrequency.minDays)
            return "early";
        if (elapsedDays <= parsedWateringFrequency.maxDays)
            return "due";
        return "late";
    }

    function setDefaultInterval(careKey, text) {
        const next = cloneSchedule();
        next[careKey].defaultIntervalDays = parseInt(text, 10) || 0;
        scheduleData = next;
    }

    function setSeasonOverride(careKey, seasonName, key, value) {
        const next = cloneSchedule();
        next[careKey].seasonal[seasonName][key] = value;
        if (key === "useOverride" && !value) {
            next[careKey].seasonal[seasonName].enabled = true;
            next[careKey].seasonal[seasonName].intervalDays = 0;
        }
        if (key === "enabled" && !value) {
            next[careKey].seasonal[seasonName].intervalDays = 0;
        }
        scheduleData = next;
    }

    function loadSchedule() {
        if (!plantData || !plantData.id || typeof plantListViewModel === "undefined" || !plantListViewModel.careSchedule) {
            scheduleData = emptySchedule();
            return;
        }
        scheduleData = plantListViewModel.careSchedule(plantData.id);
    }

    function saveSchedule() {
        if (!plantData || !plantData.id)
            return;
        if (!plantListViewModel.saveCareSchedule(plantData.id, scheduleData)) {
            statusColor = "red";
            statusText = plantListViewModel.lastError;
            return;
        }
        statusColor = "#2e7d32";
        statusText = qsTr("Care schedule saved.");
        loadSchedule();
    }

    Component.onCompleted: loadSchedule()
    onPlantDataChanged: loadSchedule()

    spacing: ui.spacing_medium

    GridLayout {
        columns: 2
        rowSpacing: ui.spacing_small
        columnSpacing: ui.spacing_medium
        Layout.fillWidth: true

        Label { text: qsTr("Last watered"); font.pixelSize: ui.point_size_small; color: "#555" }
        Label {
            text: lastCareText(val("lastWatered", ""))
            font.pixelSize: ui.point_size_small
            font.bold: true
            color: {
                switch (root.lastWateredStatus()) {
                case "early":
                    return "#2e7d32";
                case "due":
                    return "#ef6c00";
                case "late":
                    return "#c62828";
                default:
                    return palette.windowText;
                }
            }
        }

        Label { text: qsTr("Last fertilized"); font.pixelSize: ui.point_size_small; color: "#555" }
        Label { text: lastCareText(val("lastFertilized", "")); font.pixelSize: ui.point_size_small; font.bold: true }

        Label { text: qsTr("Next watering"); font.pixelSize: ui.point_size_small; color: "#555" }
        Label { text: nextCareDateText(val("lastWatered", ""), "water"); font.pixelSize: ui.point_size_small; font.bold: true }

        Label { text: qsTr("Next fertilizing"); font.pixelSize: ui.point_size_small; color: "#555" }
        Label { text: nextCareDateText(val("lastFertilized", ""), "fertilize"); font.pixelSize: ui.point_size_small; font.bold: true }
    }

    Button {
        text: schedulesExpanded ? qsTr("Hide schedules") : qsTr("Show schedules")
        Layout.fillWidth: true
        onClicked: schedulesExpanded = !schedulesExpanded
    }

    ColumnLayout {
        visible: schedulesExpanded
        spacing: ui.spacing_medium
        Layout.fillWidth: true

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: ui.spacing_small

                Label {
                    text: qsTr("Watering schedule")
                    font.pixelSize: ui.point_size_medium
                }

                Label {
                    text: qsTr("Current rule: %1").arg(activeCareText("water"))
                    color: "#555"
                    wrapMode: Text.Wrap
                }

                Label {
                    text: qsTr("Default frequency (days)")
                    color: "#555"
                }

                TextField {
                    id: waterDefaultField
                    Layout.fillWidth: true
                    placeholderText: qsTr("For example: 7")
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 365 }
                    onTextEdited: setDefaultInterval("water", text)

                    Binding on text {
                        when: !waterDefaultField.activeFocus
                        value: intervalText(careMap("water").defaultIntervalDays)
                    }
                }

                Label {
                    text: qsTr("Seasonal overrides (optional)")
                    color: "#555"
                }

                Repeater {
                    model: root.seasonNames
                    delegate: ColumnLayout {
                        readonly property string seasonName: modelData
                        readonly property var seasonConfig: root.seasonMap("water", seasonName)

                        Layout.fillWidth: true
                        spacing: ui.spacing_small

                        CheckBox {
                            text: root.seasonLabel(seasonName) + qsTr(" override")
                            checked: seasonConfig.useOverride
                            onToggled: root.setSeasonOverride("water", seasonName, "useOverride", checked)
                        }

                        RowLayout {
                            visible: seasonConfig.useOverride
                            Layout.fillWidth: true
                            spacing: ui.spacing_medium

                            CheckBox {
                                text: qsTr("Enabled")
                                checked: seasonConfig.enabled
                                onToggled: root.setSeasonOverride("water", seasonName, "enabled", checked)
                            }

                            TextField {
                                id: waterSeasonIntervalField
                                Layout.fillWidth: true
                                visible: seasonConfig.enabled
                                placeholderText: qsTr("Days between watering")
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 365 }
                                onTextEdited: root.setSeasonOverride("water", seasonName, "intervalDays", parseInt(text, 10) || 0)

                                Binding on text {
                                    when: !waterSeasonIntervalField.activeFocus
                                    value: root.intervalText(seasonConfig.intervalDays)
                                }
                            }
                        }
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: ui.spacing_small

                Label {
                    text: qsTr("Fertilizing schedule")
                    font.pixelSize: ui.point_size_medium
                }

                Label {
                    text: qsTr("Current rule: %1").arg(activeCareText("fertilize"))
                    color: "#555"
                    wrapMode: Text.Wrap
                }

                Label {
                    text: qsTr("Default frequency (days)")
                    color: "#555"
                }

                TextField {
                    id: fertilizeDefaultField
                    Layout.fillWidth: true
                    placeholderText: qsTr("For example: 14")
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 365 }
                    onTextEdited: setDefaultInterval("fertilize", text)

                    Binding on text {
                        when: !fertilizeDefaultField.activeFocus
                        value: intervalText(careMap("fertilize").defaultIntervalDays)
                    }
                }

                Label {
                    text: qsTr("Seasonal overrides (optional)")
                    color: "#555"
                }

                Repeater {
                    model: root.seasonNames
                    delegate: ColumnLayout {
                        readonly property string seasonName: modelData
                        readonly property var seasonConfig: root.seasonMap("fertilize", seasonName)

                        Layout.fillWidth: true
                        spacing: ui.spacing_small

                        CheckBox {
                            text: root.seasonLabel(seasonName) + qsTr(" override")
                            checked: seasonConfig.useOverride
                            onToggled: root.setSeasonOverride("fertilize", seasonName, "useOverride", checked)
                        }

                        RowLayout {
                            visible: seasonConfig.useOverride
                            Layout.fillWidth: true
                            spacing: ui.spacing_medium

                            CheckBox {
                                text: qsTr("Enabled")
                                checked: seasonConfig.enabled
                                onToggled: root.setSeasonOverride("fertilize", seasonName, "enabled", checked)
                            }

                            TextField {
                                id: fertilizeSeasonIntervalField
                                Layout.fillWidth: true
                                visible: seasonConfig.enabled
                                placeholderText: qsTr("Days between fertilizing")
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 365 }
                                onTextEdited: root.setSeasonOverride("fertilize", seasonName, "intervalDays", parseInt(text, 10) || 0)

                                Binding on text {
                                    when: !fertilizeSeasonIntervalField.activeFocus
                                    value: root.intervalText(seasonConfig.intervalDays)
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            text: statusText
            visible: text.length > 0
            color: statusColor
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Button {
            text: qsTr("Save care schedule")
            Layout.fillWidth: true
            onClicked: saveSchedule()
        }
    }
}
