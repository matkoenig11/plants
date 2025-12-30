import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: root

    ListModel {
        id: plantModel
        ListElement { name: "Monstera"; status: "Water today"; last: "2d ago"; type: "Tropical" }
        ListElement { name: "Snake Plant"; status: "Good"; last: "5d ago"; type: "Low light" }
        ListElement { name: "Fiddle Leaf"; status: "Water tomorrow"; last: "1d ago"; type: "Bright" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            text: qsTr("Plants")
            font.pixelSize: 22
            padding: 16
        }

        ListView {
            id: listView
            model: plantModel
            clip: true
            delegate: Item {
                width: ListView.view.width
                height: 88

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 12
                    radius: 12
                    color: "#ffffff"
                    border.color: "#e6e6e0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            width: 48
                            height: 48
                            radius: 12
                            color: "#e5f3ec"
                            Label {
                                anchors.centerIn: parent
                                text: name.charAt(0)
                                font.pixelSize: 18
                                color: "#2e9a5b"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label { id: titleLabel; text: name; font.pixelSize: 17; font.bold: true }
                            Label { text: type; font.pixelSize: 13; color: "#666" }
                            RowLayout {
                                spacing: 6
                                Rectangle {
                                    radius: 8
                                    color: status === "Water today" ? "#fff5e6" : "#eef2ff"
                                    border.color: status === "Water today" ? "#ffb347" : "#cfd6ff"
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        padding: 6
                                        text: status
                                        font.pixelSize: 12
                                    }
                                }
                                Label { text: "Last watered " + last; font.pixelSize: 12; color: "#777" }
                            }
                        }

                        Button {
                            text: qsTr("Water")
                            highlighted: true
                            onClicked: console.log("Watered", name)
                        }
                    }
                }
            }
        }
    }
}
