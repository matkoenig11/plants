import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 1.15

Page {
    required property int startIndex
    property var stackView: null
    property var plantModel // inject or access singleton

    SwipeView {
        id: pager
        anchors.fill: parent
        currentIndex: startIndex

        Repeater {
            model: plantModel
            PlantDetailPage {
                // if plantModel is a ListModel / QAbstractListModel, you can bind roles here
                plantId: model.id
                plantName: model.name
                // ...more roles
                Component.onCompleted: {
                    console.log("Loaded plant:", plantName)
                }
            }
        }
    }

    header: ToolBar {
        RowLayout {
            ToolButton { text: "◀"; onClicked: pager.decrementCurrentIndex() }  // :contentReference[oaicite:4]{index=4}
            Label { text: (pager.currentIndex+1) + " / " + plantModel.count }
            ToolButton { text: "▶"; onClicked: pager.incrementCurrentIndex() }  // :contentReference[oaicite:5]{index=5}
        }
    }
}
