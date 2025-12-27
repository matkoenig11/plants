import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root

    property var plantModel
    signal plantSelected(var plantData)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Plants"
            font.pixelSize: 18
        }

        ListView {
            id: plantList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.plantModel
            delegate: ItemDelegate {
                width: ListView.view.width
                text: model.name
                highlighted: ListView.isCurrentItem
                onClicked: {
                    plantList.currentIndex = index
                    root.plantSelected({
                        id: model.id,
                        name: model.name,
                        scientificName: model.scientificName,
                        plantType: model.plantType,
                        winterLocation: model.winterLocation,
                        summerLocation: model.summerLocation,
                        lightRequirement: model.lightRequirement,
                        wateringFrequency: model.wateringFrequency,
                        wateringNotes: model.wateringNotes,
                        humidityPreference: model.humidityPreference,
                        soilType: model.soilType,
                        potSize: model.potSize,
                        lastWatered: model.lastWatered,
                        fertilizingSchedule: model.fertilizingSchedule,
                        lastFertilized: model.lastFertilized,
                        pruningTime: model.pruningTime,
                        pruningNotes: model.pruningNotes,
                        lastPruned: model.lastPruned,
                        growthRate: model.growthRate,
                        currentHealthStatus: model.currentHealthStatus,
                        issuesPests: model.issuesPests,
                        temperatureTolerance: model.temperatureTolerance,
                        toxicToPets: model.toxicToPets,
                        acquiredDate: model.acquiredDate,
                        source: model.source,
                        notes: model.notes
                    })
                }
            }
        }
    }
}
