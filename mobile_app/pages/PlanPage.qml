import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    property int selectedDay: 0

    ListModel {
        id: taskModel
        ListElement { plant: "Monstera"; action: "Water"; when: "Today 18:00"; status: "due" }
        ListElement { plant: "Snake Plant"; action: "Fertilize"; when: "Thu"; status: "upcoming" }
        ListElement { plant: "Fiddle Leaf"; action: "Prune"; when: "Sat"; status: "upcoming" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Column {
            spacing: 8
            Label { text: qsTr("This week"); font.pixelSize: 22 }
            Label { text: qsTr("Agenda across plants"); color: "#666" }
        }

        Flickable {
            contentWidth: dayRow.implicitWidth
            flickableDirection: Flickable.HorizontalFlick
            interactive: dayRow.implicitWidth > width
            clip: true
            Layout.fillWidth: true
            Layout.preferredHeight: 54

            Row {
                id: dayRow
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                Repeater {
                    model: 7
                    delegate: Rectangle {
                        width: 64; height: 40; radius: 12
                        color: index === selectedDay ? "#2e9a5b" : "#ffffff"
                        border.color: index === selectedDay ? "#2e9a5b" : "#dddddd"
                        Label {
                            anchors.centerIn: parent
                            text: ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"][index]
                            color: index === selectedDay ? "white" : "#333"
                            font.pixelSize: 14
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: selectedDay = index
                        }
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: taskModel
            clip: true
            delegate: Item {
                width: ListView.view.width
                height: 82

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 12
                    radius: 14
                    color: "#ffffff"
                    border.color: "#e6e6e0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        CheckBox {
                            checked: status === "done"
                            onClicked: console.log("Complete toggled", plant, !checked)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: plant + " · " + action
                                font.pixelSize: 16
                                font.bold: true
                                color: status === "due" ? "#d9534f" : "#222"
                            }
                            Label {
                                text: when
                                font.pixelSize: 13
                                color: status === "due" ? "#d9534f" : "#666"
                            }
                        }
                    }
                }
            }
        }
    }
}
