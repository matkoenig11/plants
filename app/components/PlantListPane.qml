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
                required property int id
                required property string name
                required property string scientificName
                required property string plantType
                required property string winterLocation
                required property string summerLocation
                required property string lightRequirement
                required property string wateringFrequency
                required property string wateringNotes
                required property string humidityPreference
                required property string soilType
                required property string potSize
                required property string lastWatered
                required property string fertilizingSchedule
                required property string lastFertilized
                required property string pruningTime
                required property string pruningNotes
                required property string lastPruned
                required property string growthRate
                required property string currentHealthStatus
                required property string issuesPests
                required property string temperatureTolerance
                required property string toxicToPets
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
                        winterLocation: winterLocation,
                        summerLocation: summerLocation,
                        lightRequirement: lightRequirement,
                        wateringFrequency: wateringFrequency,
                        wateringNotes: wateringNotes,
                        humidityPreference: humidityPreference,
                        soilType: soilType,
                        potSize: potSize,
                        lastWatered: lastWatered,
                        fertilizingSchedule: fertilizingSchedule,
                        lastFertilized: lastFertilized,
                        pruningTime: pruningTime,
                        pruningNotes: pruningNotes,
                        lastPruned: lastPruned,
                        growthRate: growthRate,
                        currentHealthStatus: currentHealthStatus,
                        issuesPests: issuesPests,
                        temperatureTolerance: temperatureTolerance,
                        toxicToPets: toxicToPets,
                        acquiredDate: acquiredDate,
                        source: source,
                        notes: notes
                    })
                }
            }
        }
    }
}
