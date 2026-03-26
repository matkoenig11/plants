import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GridLayout {
    property var plantData: ({})
    columns: 2
    rowSpacing: 6
    columnSpacing: 12

    function val(key, fallback) {
        if (plantData && plantData[key] !== undefined && plantData[key] !== "") {
            return plantData[key];
        }
        return fallback !== undefined ? fallback : "";
    }

    Label { text: qsTr("Light"); font.pixelSize: 14; color: "#555" }
    Label { text: val("lightRequirement", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Indoor"); font.pixelSize: 14; color: "#555" }
    Label { text: val("indoor", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Flowering season"); font.pixelSize: 14; color: "#555" }
    Label { text: val("floweringSeason", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Humidity"); font.pixelSize: 14; color: "#555" }
    Label { text: val("humidityPreference", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Watering"); font.pixelSize: 14; color: "#555" }
    Label { text: val("wateringFrequency", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Watering notes"); font.pixelSize: 14; color: "#555" }
    Label { text: val("wateringNotes", qsTr("Not set")); font.pixelSize: 14; font.bold: true; wrapMode: Text.Wrap }

    Label { text: qsTr("Fertilizing"); font.pixelSize: 14; color: "#555" }
    Label { text: val("fertilizingSchedule", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Soil"); font.pixelSize: 14; color: "#555" }
    Label { text: val("soilType", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Growth rate"); font.pixelSize: 14; color: "#555" }
    Label { text: val("growthRate", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Temperature"); font.pixelSize: 14; color: "#555" }
    Label { text: val("temperatureTolerance", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Toxic to pets"); font.pixelSize: 14; color: "#555" }
    Label { text: val("toxicToPets", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Poisonous to pets"); font.pixelSize: 14; color: "#555" }
    Label { text: val("poisonousToPets", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }

    Label { text: qsTr("Poisonous to humans"); font.pixelSize: 14; color: "#555" }
    Label { text: val("poisonousToHumans", qsTr("Unknown")); font.pixelSize: 14; font.bold: true }
}
