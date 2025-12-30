import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: root

    signal plantSelected(var plantData, int index)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: qsTr("Plants"); font.pixelSize: 20; Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); onClicked: plantListViewModel.refresh() }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: plantListViewModel
            delegate: ItemDelegate {
                width: ListView.view.width
                required property int id
                required property string name
                required property string plantType
                required property string lightRequirement
                required property string wateringFrequency
                text: name
                onClicked: {
                    var idx = ListView.view ? ListView.view.indexAt(width / 2, y + height / 2) : -1
                    root.plantSelected({
                        id: id,
                        name: name,
                        plantType: plantType,
                        lightRequirement: lightRequirement,
                        wateringFrequency: wateringFrequency
                    }, idx)
                }
            }
            clip: true
            ScrollBar.vertical: ScrollBar { }
        }
    }
}
