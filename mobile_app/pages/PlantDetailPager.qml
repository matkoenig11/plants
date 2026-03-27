import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "details"

Page {
    id: pagerPage
    required property int startIndex
    property var stackView: null
    property var plantModel // inject or access singleton

    SwipeView {
        id: pager
        anchors.fill: parent
        
        currentIndex: startIndex

        Repeater {
            model: plantModel
            Item {
                id: pageWrapper
                width: pager.width
                height: pager.height

                readonly property bool shouldLoad: Math.abs(index - pager.currentIndex) <= 1
                readonly property var pageData: ({
                    id: id,
                    name: name,
                    scientificName: scientificName,
                    plantType: plantType,
                    lightRequirement: lightRequirement,
                    indoor: indoor,
                    floweringSeason: floweringSeason,
                    tags: tags,
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

                Loader {
                    anchors.fill: parent
                    active: pageWrapper.shouldLoad
                    asynchronous: true
                    sourceComponent: detailPageComponent
                }

                Component {
                    id: detailPageComponent

                    PlantDetailPage {
                        stackView: pagerPage.stackView
                        plantData: pageWrapper.pageData
                    }
                }
            }
        }
    }


    header: ToolBar {
        background: Rectangle { color: "#00a169ff"; opacity: 1 }
        RowLayout {
            spacing: 8
            ToolButton {
                text: qsTr("Back")
                onClicked: {
                    if (stackView) {
                        stackView.pop()
                    } else if (StackView.view) {
                        StackView.view.pop()
                    }
                }
            }
            ToolButton { text: "◀"; 
            onClicked: {
                if (pager.currentIndex > 0) {        
                    pager.decrementCurrentIndex() 
                } else {
                    pager.currentIndex = pager.count - 1
                    }
            }
                 }
            Label {
                text: (pager.currentIndex+1) + " / " + plantListViewModel.count
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            ToolButton { text: "▶"; onClicked: {

                    if (pager.currentIndex < pager.count - 1){
                        pager.incrementCurrentIndex()
                    } else {
                        pager.currentIndex = 0
                    }
                
            } }
        }
    }
}
