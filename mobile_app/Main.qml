import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "pages"

ApplicationWindow {
    id: window
    visible: true
    width: 390
    height: 780
    title: qsTr("Plants")
    color: "#f7f7f4"

    Material.theme: Material.Light
    Material.accent: "#2e9a5b"

    Component.onCompleted: plantListViewModel.refresh()

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: PlantListPage {
            onPlantSelected: function(plantData, index) {
                stack.push("qrc:/mobile_app/pages/PlantDetailPager.qml", {
                    stackView: stack,
                    plantModel: plantListViewModel,
                    startIndex: index
                })
            }
        }
    }
}
