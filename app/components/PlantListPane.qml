import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Frame {
    id: root
    AppConstants { id: ui }

    property var plantModel
    signal plantSelected(var plantData)

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_medium

        Label {
            text: "Plants"
            font.pixelSize: ui.point_size_medium
        }

        ListView {
            id: plantList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.plantModel
            delegate: ItemDelegate {
                width: ListView.view.width
                required property int id
                required property string name
                required property string scientificName
                required property string plantType
                required property string lightRequirement
                required property string indoor
                required property string floweringSeason
                required property string wateringFrequency
                required property string wateringNotes
                required property string humidityPreference
                required property string soilType
                required property string lastWatered
                required property string fertilizingSchedule
                required property string lastFertilized
                required property string pruningTime
                required property string pruningNotes
                required property string lastPruned
                required property string growthRate
                required property string issuesPests
                required property string temperatureTolerance
                required property string toxicToPets
                required property string poisonousToHumans
                required property string poisonousToPets
                required property string acquiredDate
                required property string source
                required property string notes

                text: name
                highlighted: ListView.isCurrentItem
                onClicked: {
                    plantList.currentIndex = index
                    root.plantSelected({
                        id: id,
                        name: name,
                        scientificName: scientificName,
                        plantType: plantType,
                        lightRequirement: lightRequirement,
                        indoor: indoor,
                        floweringSeason: floweringSeason,
                        wateringFrequency: wateringFrequency,
                        wateringNotes: wateringNotes,
                        humidityPreference: humidityPreference,
                        soilType: soilType,
                        lastWatered: lastWatered,
                        fertilizingSchedule: fertilizingSchedule,
                        lastFertilized: lastFertilized,
                        pruningTime: pruningTime,
                        pruningNotes: pruningNotes,
                        lastPruned: lastPruned,
                        growthRate: growthRate,
                        issuesPests: issuesPests,
                        temperatureTolerance: temperatureTolerance,
                        toxicToPets: toxicToPets,
                        poisonousToHumans: poisonousToHumans,
                        poisonousToPets: poisonousToPets,
                        acquiredDate: acquiredDate,
                        source: source,
                        notes: notes
                    })
                }
            }
        }
    }
}
