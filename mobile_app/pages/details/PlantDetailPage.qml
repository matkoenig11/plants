import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import "../../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var plantData: ({})
    property var stackView: null
    property var imageEntries: []

    readonly property string plainNotes: decodeNotes(plantData.notes)
    readonly property string primaryImage: imageEntries.length > 0 ? (imageEntries[0].url || "") : ""

    function rtrim(value) {
        return value ? value.replace(/\s+$/, "") : "";
    }

    function decodeNotes(notesText) {
        const marker = "\n__images__:";
        if (!notesText || notesText.indexOf(marker) === -1) {
            return notesText || "";
        }
        const markerIndex = notesText.indexOf(marker);
        return rtrim(notesText.slice(0, markerIndex));
    }

    function reloadImages() {
        if (!plantData || !plantData.id || typeof plantListViewModel === "undefined" || !plantListViewModel.plantImages) {
            imageEntries = [];
            return;
        }
        imageEntries = plantListViewModel.plantImages(plantData.id);
    }

    function addSelectedImage(fileUrl) {
        if (!fileUrl || !plantData || !plantData.id)
            return;
        if (!plantListViewModel.addPlantImage(plantData.id, fileUrl)) {
            console.log("Failed to add image:", plantListViewModel.lastError);
            return;
        }
        reloadImages();
    }

    function removePrimaryImage() {
        if (!imageEntries.length) {
            return;
        }
        const imageId = imageEntries[0].id || 0;
        if (!imageId) {
            return;
        }
        if (!plantListViewModel.removePlantImage(imageId)) {
            console.log("Failed to remove image:", plantListViewModel.lastError);
            return;
        }
        reloadImages();
    }

    function openImageSourceMenu() {
        imageSourceMenu.popup()
    }

    function openCameraCapture() {
        const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
        if (targetStack) {
            targetStack.push("qrc:/mobile_app/pages/CameraCapturePage.qml", {
                stackView: targetStack,
                onCaptured: function(filePath) {
                    addSelectedImage(filePath)
                }
            })
        }
    }

    function val(key, fallback) {
        if (plantData && plantData[key] !== undefined && plantData[key] !== "") {
            return plantData[key];
        }
        return fallback !== undefined ? fallback : "";
    }

    function deleteCurrentPlant() {
        if (!plantData || !plantData.id) {
            return;
        }
        if (!plantListViewModel.removePlant(plantData.id)) {
            console.log("Failed to delete plant:", plantListViewModel.lastError)
            return;
        }

        const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
        if (targetStack) {
            targetStack.pop()
        }
    }

    onPlantDataChanged: reloadImages()
    Component.onCompleted: reloadImages()

    FileDialog {
        id: imagePicker
        title: qsTr("Select image")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)")]
        fileMode: FileDialog.OpenFile
        onAccepted: addSelectedImage(selectedFile)
    }

    Menu {
        id: imageSourceMenu

        Action {
            text: qsTr("Gallery")
            onTriggered: imagePicker.open()
        }

        Action {
            text: qsTr("Take Photo")
            onTriggered: openCameraCapture()
        }

        Action {
            text: qsTr("Remove Photo")
            enabled: imageEntries.length > 0
            onTriggered: removePrimaryImage()
        }
    }

    MessageDialog {
        id: deletePlantDialog
        title: qsTr("Delete Plant")
        text: qsTr("Delete this plant and its local journal entries, reminders, images, and schedules?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: deleteCurrentPlant()
    }

    ScrollView {
        anchors.fill: parent
        // contentWidth: availableWidth
        // Rectangle {
        //     anchors.fill: parent
        //     color: "red"
        // }

        ColumnLayout {
            anchors.fill: parent
            spacing: ui.spacing_large
            Label {
                text: plantData.name.length ? plantData.name : qsTr("Unknown Plant") 
                font.pixelSize: ui.point_size_xlarge
                Layout.margins: ui.margin_medium
            }
            Item {
                Layout.fillWidth: true
                implicitHeight: plantImage.visible ? 220 : 0

                Image {
                    id: plantImage
                    anchors.fill: parent
                    anchors.margins: ui.margin_large
                    source: root.primaryImage
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    smooth: true
                    clip: true
                    visible: root.primaryImage.length > 0
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: plantImage.visible
                    onPressAndHold: openImageSourceMenu()
                }

                Button {
                    text: qsTr("Add Image")
                    anchors.centerIn: parent
                    visible: !plantImage.visible
                    onClicked: openImageSourceMenu()
                }
            }
            GeneralInfoSection { 
                // data: plantData 
                width: parent.width
            }
            Button {
                text: qsTr("Edit Plant Info")
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.push("qrc:/mobile_app/pages/PlantDetailEditPage.qml", {
                            plantData: plantData,
                            stackView: targetStack
                        })
                    } 
                }
            }
            Button {
                text: qsTr("Journal")
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.push("qrc:/mobile_app/pages/JournalPage.qml", {
                            plantId: plantData.id,
                            plantName: plantData.name,
                            stackView: targetStack
                        })
                    }
                }
            }
            Button {
                text: qsTr("Delete Plant")
                onClicked: deletePlantDialog.open()
            }
            Label {
                text: qsTr("Care") + ":" + plantData.scientificName
                font.pixelSize: ui.point_size_large
            }
            GeneralCareSection {
                plantData: root.plantData
                Layout.margins: ui.margin_medium
            }// data: plantData }
            
            UpdateSection { 
                plantData: root.plantData
                Layout.margins: ui.margin_medium
            }// data: plantData }
            Label {
                text: qsTr("Notes")
                font.pixelSize: ui.point_size_large
                Layout.margins: ui.margin_medium
            }
            Text {
                text: plainNotes.length ? plainNotes : qsTr("No notes yet.")
                wrapMode: Text.Wrap
                Layout.margins: ui.margin_medium
            }
            Repeater {
                model: imageEntries
                delegate: Rectangle {
                    width: parent ? parent.width : 320
                    height: ui.detail_image_height
                    color: "#f0f0f0"
                    radius: 6
                    border.color: "#ccc"
                    Image {
                        anchors.fill: parent
                        anchors.margins: ui.margin_small
                        source: modelData.url || ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }
                }
            }

            // Item { Layout.fillWidth: true; Layout.preferredHeight: 12 }
        }
    }
}
