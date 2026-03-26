import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }

    property int plantId: -1
    property string plantName: ""
    property var stackView: null

    function loadJournal() {
        if (typeof journalEntryViewModel === "undefined" || plantId <= 0)
            return;
        journalEntryViewModel.setPlantId(plantId);
        journalEntryViewModel.refresh();
    }

    Component.onCompleted: loadJournal()
    onPlantIdChanged: loadJournal()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: ui.spacing_medium

            ToolButton {
                text: qsTr("Back")
                onClicked: {
                    if (root.stackView) {
                        root.stackView.pop()
                    } else if (StackView.view) {
                        StackView.view.pop()
                    }
                }
            }

            Label {
                text: root.plantName.length > 0 ? root.plantName : qsTr("Journal")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_medium
        anchors.margins: ui.margin_large

        Label {
            text: qsTr("Journal")
            font.pixelSize: ui.point_size_large
        }

        Label {
            text: qsTr("Watering and fertilizing history")
            font.pixelSize: ui.point_size_small
            color: "#666666"
        }

        Label {
            text: journalEntryViewModel.lastError
            visible: text.length > 0
            color: "red"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ListView {
            id: journalList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: journalEntryViewModel
            spacing: ui.spacing_small

            delegate: ItemDelegate {
                width: ListView.view.width
                required property string entryType
                required property string entryDate
                required property string notes

                readonly property bool supportedType: entryType === "Water" || entryType === "Fertilize"
                visible: supportedType
                height: supportedType ? implicitHeight : 0

                contentItem: ColumnLayout {
                    spacing: ui.spacing_small

                    Label {
                        text: entryType + " • " + entryDate
                        font.pixelSize: ui.point_size_medium
                    }

                    Label {
                        text: notes
                        visible: notes.length > 0
                        wrapMode: Text.Wrap
                        color: "#666666"
                    }
                }
            }
        }

        Label {
            text: qsTr("No journal entries yet.")
            visible: journalList.count === 0
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            color: "#666666"
        }
    }
}
