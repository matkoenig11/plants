import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 
import "pages"
import "../ui"

ApplicationWindow {
    id: window
    AppConstants { id: ui }
    visible: true
    width: ui.mobile_app_width
    height: ui.mobile_app_height
    title: qsTr("Plants")
    // color: "#963889ff"

    Material.theme: Material.Dark
    Material.accent: "#2e9a5b"
    // Material.background: "#00000000"

    function returnToMainView() {
        if (stack.depth <= 1)
            return false

        stack.pop(null)
        return true
    }

    Component.onCompleted: plantListViewModel.refresh()

    Shortcut {
        context: Qt.ApplicationShortcut
        sequences: [StandardKey.Back]
        enabled: stack.depth > 1
        onActivated: window.returnToMainView()
    }

    header: ToolBar {
        Material.primary: "#000000ff"
        Material.background: "transparent"
        Material.foreground: "#ffffff"
        background: Rectangle { color: "#00a169ff"; opacity: 1 }
        RowLayout {
            anchors.fill: parent
            spacing: ui.spacing_medium
            ToolButton {
                text: "\u2630" // simple hamburger
                onClicked: mainMenu.open()
            }
            Label {
                text: qsTr("Plants")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
            }
        }
        Menu {
            id: mainMenu
            Action {
                text: qsTr("Import SQLite")
                onTriggered: sqliteDialog.open()
            }
            Action {
                text: qsTr("Database Connection")
                onTriggered: stack.push("qrc:/mobile_app/pages/DatabaseConnectionPage.qml", {
                    stackView: stack
                })
            }
            Action {
                text: qsTr("Library")
                onTriggered: stack.push("qrc:/mobile_app/pages/LibraryPage.qml", {
                    stackView: stack
                })
            }
            Action {
                text: qsTr("Tags")
                onTriggered: stack.push("qrc:/mobile_app/pages/TagManagementPage.qml", {
                    stackView: stack
                })
            }
        }
    }

    FileDialog {
        id: sqliteDialog
        title: qsTr("Select SQLite database")
        // nameFilters: [qsTr("SQLite files (*.sqlite *.db)")]
        nameFilters: [qsTr("Txt files (*.sqlite *.db *.txt)")]
        onAccepted: {
            console.log("Selected file: " + selectedFile)
            if (plantListViewModel.importFromSqlite(selectedFile)) {
                plantListViewModel.refresh()
            } else {
                console.log("Import failed: " + plantListViewModel.lastError)
            }
        }
    }


    StackView {
        id: stack
        anchors.fill: parent
        initialItem: PlantListPage {
            stackView: stack
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
