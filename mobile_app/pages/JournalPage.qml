import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {

    ListModel {
        id: journalModel
        ListElement { action: "Water"; plant: "Monstera"; note: "Thorough soak"; when: "Today 09:00" }
        ListElement { action: "Prune"; plant: "Fiddle Leaf"; note: "Removed yellow leaf"; when: "Yesterday" }
        ListElement { action: "Fertilize"; plant: "Snake Plant"; note: "Half strength"; when: "Sun" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Column {
            padding: 16
            spacing: 8
            Label { text: qsTr("Journal"); font.pixelSize: 22 }
            RowLayout {
                spacing: 8
                Repeater {
                    model: ["All", "Water", "Prune", "Fertilize", "Repot"]
                    delegate: CheckBox {
                        text: modelData
                        checked: index === 0
                        onClicked: console.log("Filter", modelData)
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: journalModel
            clip: true
            delegate: Item {
                width: ListView.view.width
                height: 80

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

                        Rectangle {
                            width: 36; height: 36; radius: 10
                            color: "#e5f3ec"
                            Label {
                                anchors.centerIn: parent
                                text: action.charAt(0)
                                color: "#2e9a5b"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: action + " · " + plant; font.pixelSize: 16; font.bold: true }
                            Label { text: note; color: "#555"; font.pixelSize: 13 }
                            Label { text: when; color: "#777"; font.pixelSize: 12 }
                        }
                    }
                }
            }
        }
    }
}
