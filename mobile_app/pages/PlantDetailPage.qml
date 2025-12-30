import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: root
    property int plantId: 0
    property string plantName: ""
    property string plantType: ""
    property string plantLight: ""
    property string plantWater: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            height: 140
            color: "#e5f3ec"
            Label {
                anchors.centerIn: parent
                text: plantName.length ? plantName : qsTr("Unknown Plant")
                font.pixelSize: 18
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            GroupBox {
                title: qsTr("Care")
                Layout.fillWidth: true
                Column {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6
                    Label { text: qsTr("Light: %1").arg(plantLight.length ? plantLight : qsTr("Unknown")) }
                    Label { text: qsTr("Water: %1").arg(plantWater.length ? plantWater : qsTr("Unknown")) }
                }
            }
        }
    }
}
