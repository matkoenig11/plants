import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {

    ListModel {
        id: reminderModel
        ListElement { plant: "Monstera"; action: "Water"; due: "Today 18:00"; enabled: true }
        ListElement { plant: "Snake Plant"; action: "Fertilize"; due: "Thu"; enabled: false }
        ListElement { plant: "Fiddle Leaf"; action: "Prune"; due: "Sat"; enabled: true }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label { text: qsTr("Reminders"); font.pixelSize: 22; padding: 16 }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: reminderModel
            clip: true
            delegate: Item {
                width: ListView.view.width
                height: 72

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 12
                    radius: 12
                    color: "#ffffff"
                    border.color: "#e6e6e0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: plant + " · " + action; font.pixelSize: 16; font.bold: true }
                            Label { text: "Due " + due; font.pixelSize: 13; color: "#666" }
                        }

                        Switch {
                            checked: enabled
                            onToggled: console.log("Reminder toggled", plant, checked)
                        }
                    }
                }
            }
        }
    }
}
