import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../utils/CareFrequency.js" as CareFrequency

GridLayout {
    property var plantData: ({})
    columns: 2
    rowSpacing: 6
    columnSpacing: 12
    Layout.fillWidth: true
    readonly property string rawWateringFrequency: plantData && plantData.wateringFrequency ? String(plantData.wateringFrequency) : ""
    readonly property var parsedWateringFrequency: CareFrequency.parseWateringFrequency(rawWateringFrequency)

    component FieldLabel: Label {
        font.pixelSize: 14
        color: "#555"
        Layout.alignment: Qt.AlignTop
    }

    component FieldValue: Label {
        font.pixelSize: 14
        font.bold: true
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
    }

    function val(key, fallback) {
        if (plantData && plantData[key] !== undefined && plantData[key] !== "") {
            return plantData[key];
        }
        return fallback !== undefined ? fallback : "";
    }

    FieldLabel { text: qsTr("Light") }
    FieldValue { text: val("lightRequirement", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Indoor") }
    FieldValue { text: val("indoor", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Flowering season") }
    FieldValue { text: val("floweringSeason", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Humidity") }
    FieldValue { text: val("humidityPreference", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Watering") }
    FieldValue { text: val("wateringFrequency", qsTr("Unknown")) }
    Label {
        visible: rawWateringFrequency.trim().length > 0 && !parsedWateringFrequency.valid
        text: qsTr("Can't parse watering frequency for watering-status colors.")
        color: "#c62828"
        wrapMode: Text.Wrap
        Layout.columnSpan: 2
        Layout.fillWidth: true
    }

    FieldLabel { text: qsTr("Watering notes") }
    FieldValue { text: val("wateringNotes", qsTr("Not set")) }

    FieldLabel { text: qsTr("Fertilizing") }
    FieldValue { text: val("fertilizingSchedule", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Soil") }
    FieldValue { text: val("soilType", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Growth rate") }
    FieldValue { text: val("growthRate", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Temperature") }
    FieldValue { text: val("temperatureTolerance", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Toxic to pets") }
    FieldValue { text: val("toxicToPets", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Poisonous to pets") }
    FieldValue { text: val("poisonousToPets", qsTr("Unknown")) }

    FieldLabel { text: qsTr("Poisonous to humans") }
    FieldValue { text: val("poisonousToHumans", qsTr("Unknown")) }
}
