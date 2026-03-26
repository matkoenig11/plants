import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var plantData: ({})
    property var stackView: null
    property var imageEntries: []
    readonly property bool isNewPlant: !plantData || !plantData.id
    readonly property int formWidth: Math.min(width - (ui.margin_large * 2), 760)
    readonly property int fieldHeight: 54
    readonly property int formPadding: ui.margin_large + ui.margin_medium
    readonly property color pageBackgroundColor: "#10171d"
    readonly property color cardBackgroundColor: "#18222c"
    readonly property color cardBorderColor: "#2b3946"
    readonly property color sectionTextColor: "#dbe6ef"
    readonly property color secondaryTextColor: "#93a5b5"
    readonly property color tileBackgroundColor: "#202c37"
    readonly property color tileBorderColor: "#31404d"

    component PlantField: TextField {
        Layout.fillWidth: true
        implicitHeight: root.fieldHeight
        font.pixelSize: ui.point_size_medium
        selectByMouse: true
    }

    component SectionHeader: Label {
        Layout.fillWidth: true
        font.pixelSize: ui.point_size_medium
        font.bold: true
        topPadding: ui.spacing_small
        bottomPadding: ui.spacing_small
        color: root.sectionTextColor
    }

    background: Rectangle {
        color: root.pageBackgroundColor
    }

    function decodeNotes(notesText) {
        const imagesMarker = "\n__images__:";
        if (!notesText || notesText.indexOf(imagesMarker) === -1) {
            return notesText || "";
        }
        const markerIndex = notesText.indexOf(imagesMarker);
        return notesText.slice(0, markerIndex).replace(/\s+$/, "");
    }

    function reloadImages() {
        if (!plantData || !plantData.id || typeof plantListViewModel === "undefined" || !plantListViewModel.plantImages) {
            imageEntries = [];
            return;
        }
        imageEntries = plantListViewModel.plantImages(plantData.id);
    }

    function addImages(paths) {
        if (!Array.isArray(paths) || !plantData || !plantData.id)
            return;
        for (let i = 0; i < paths.length; ++i) {
            if (!plantListViewModel.addPlantImage(plantData.id, paths[i])) {
                statusLabel.text = plantListViewModel.lastError;
                return;
            }
        }
        statusLabel.text = "";
        reloadImages();
    }

    function openCameraCapture() {
        const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
        if (targetStack) {
            targetStack.push("qrc:/mobile_app/pages/CameraCapturePage.qml", {
                stackView: targetStack,
                onCaptured: function(filePath) {
                    addImages([filePath])
                }
            })
        }
    }

    function removeImage(imageId) {
        if (!plantListViewModel.removePlantImage(imageId)) {
            statusLabel.text = plantListViewModel.lastError;
            return;
        }
        statusLabel.text = "";
        reloadImages();
    }

    function removePrimaryImage() {
        if (!imageEntries.length) {
            return;
        }
        removeImage(imageEntries[0].id || 0)
    }

    Component.onCompleted: {
        notesField.text = decodeNotes(plantData.notes);
        reloadImages();
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: qsTr("Cancel")
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.pop();
                    }
                }
            }
            Label {
                text: root.isNewPlant ? qsTr("Add Plant") : qsTr("Edit Plant")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            ToolButton {
                text: qsTr("Save")
                onClicked: saveChanges()
            }
        }
    }

    FileDialog {
        id: imagePicker
        title: qsTr("Select images")
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.gif)")]
        fileMode: FileDialog.OpenFiles
        onAccepted: addImages(selectedFiles)
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

    ScrollView {
        id: editScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Column {
            width: editScroll.availableWidth
            spacing: ui.spacing_large

            Item {
                width: 1
                height: ui.margin_large
            }

            Rectangle {
                width: root.formWidth
                height: implicitHeight
                implicitHeight: formCardLayout.implicitHeight + (root.formPadding * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 22
                color: root.cardBackgroundColor
                border.color: root.cardBorderColor

                ColumnLayout {
                    id: formCardLayout
                    x: root.formPadding
                    y: root.formPadding
                    width: parent.width - (root.formPadding * 2)
                    spacing: ui.spacing_medium

                    Label {
                        text: root.isNewPlant ? qsTr("Create A New Plant") : qsTr("Edit Plant Details")
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: ui.point_size_large
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Wider fields for quicker editing on mobile.")
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        color: root.secondaryTextColor
                    }

                    SectionHeader { text: qsTr("Identity") }
                    PlantField { id: nameField; placeholderText: qsTr("Name *"); text: plantData.name || "" }
                    PlantField { id: scientificNameField; placeholderText: qsTr("Scientific name"); text: plantData.scientificName || "" }
                    PlantField { id: plantTypeField; placeholderText: qsTr("Plant type"); text: plantData.plantType || "" }
                    PlantField { id: sourceField; placeholderText: qsTr("Source"); text: plantData.source || "" }
                    PlantField { id: acquiredDateField; placeholderText: qsTr("Acquired date (YYYY-MM-DD)"); text: plantData.acquiredDate || "" }

                    SectionHeader { text: qsTr("Placement") }
                    PlantField { id: lightRequirementField; placeholderText: qsTr("Light requirement"); text: plantData.lightRequirement || "" }
                    PlantField { id: indoorField; placeholderText: qsTr("Indoor"); text: plantData.indoor || "" }
                    PlantField { id: floweringSeasonField; placeholderText: qsTr("Flowering season"); text: plantData.floweringSeason || "" }
                    PlantField { id: temperatureToleranceField; placeholderText: qsTr("Temperature tolerance"); text: plantData.temperatureTolerance || "" }

                    SectionHeader { text: qsTr("Care") }
                    PlantField { id: wateringFrequencyField; placeholderText: qsTr("Watering frequency"); text: plantData.wateringFrequency || "" }
                    PlantField { id: wateringNotesField; placeholderText: qsTr("Watering notes"); text: plantData.wateringNotes || "" }
                    PlantField { id: lastWateredField; placeholderText: qsTr("Last watered (YYYY-MM-DD)"); text: plantData.lastWatered || "" }
                    PlantField { id: fertilizingScheduleField; placeholderText: qsTr("Fertilizing schedule"); text: plantData.fertilizingSchedule || "" }
                    PlantField { id: lastFertilizedField; placeholderText: qsTr("Last fertilized (YYYY-MM-DD)"); text: plantData.lastFertilized || "" }
                    PlantField { id: pruningTimeField; placeholderText: qsTr("Pruning time"); text: plantData.pruningTime || "" }
                    PlantField { id: pruningNotesField; placeholderText: qsTr("Pruning notes"); text: plantData.pruningNotes || "" }
                    PlantField { id: lastPrunedField; placeholderText: qsTr("Last pruned (YYYY-MM-DD)"); text: plantData.lastPruned || "" }

                    SectionHeader { text: qsTr("Environment") }
                    PlantField { id: humidityPreferenceField; placeholderText: qsTr("Humidity preference"); text: plantData.humidityPreference || "" }
                    PlantField { id: soilTypeField; placeholderText: qsTr("Soil type"); text: plantData.soilType || "" }
                    PlantField { id: growthRateField; placeholderText: qsTr("Growth rate"); text: plantData.growthRate || "" }

                    SectionHeader { text: qsTr("Health") }
                    PlantField { id: issuesPestsField; placeholderText: qsTr("Issues / pests"); text: plantData.issuesPests || "" }
                    PlantField { id: toxicToPetsField; placeholderText: qsTr("Toxic to pets"); text: plantData.toxicToPets || "" }
                    PlantField { id: poisonousToPetsField; placeholderText: qsTr("Poisonous to pets"); text: plantData.poisonousToPets || "" }
                    PlantField { id: poisonousToHumansField; placeholderText: qsTr("Poisonous to humans"); text: plantData.poisonousToHumans || "" }

                    SectionHeader { text: qsTr("Notes") }
                    TextArea {
                        id: notesField
                        placeholderText: qsTr("Notes")
                        wrapMode: TextArea.Wrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        font.pixelSize: ui.point_size_medium
                    }

                    SectionHeader { text: qsTr("Images") }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ui.spacing_medium

                        Label {
                            text: qsTr("Plant photos")
                            Layout.fillWidth: true
                            font.pixelSize: ui.point_size_medium
                        }

                        Button {
                            text: qsTr("Add")
                            enabled: !root.isNewPlant
                            onClicked: imageSourceMenu.popup()
                        }
                    }

                    Label {
                        visible: root.isNewPlant
                        text: qsTr("Save the plant first before adding images.")
                        color: root.secondaryTextColor
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: root.imageEntries
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: ui.image_tile_height
                            radius: 12
                            color: root.tileBackgroundColor
                            border.color: root.tileBorderColor
                            readonly property int imageId: modelData.id || 0
                            readonly property string imagePath: modelData.path || ""
                            readonly property string imageUrl: modelData.url || ""

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: ui.margin_medium
                                spacing: ui.spacing_small

                                Image {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: ui.thumbnail_size_medium - ui.margin_small
                                    source: imageUrl
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: ui.spacing_medium

                                    Label {
                                        text: imagePath
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }

                                    Button {
                                        text: qsTr("Remove")
                                        onClicked: removeImage(imageId)
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        id: statusLabel
                        color: "red"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Button {
                        text: qsTr("Save changes")
                        Layout.fillWidth: true
                        implicitHeight: 56
                        onClicked: saveChanges()
                    }
                }
            }

            Item {
                width: 1
                height: ui.margin_large * 2
            }
        }
    }

    function saveChanges() {
        if (!nameField.text.trim()) {
            statusLabel.text = qsTr("Name is required.");
            return;
        }

        const payload = {
            name: nameField.text,
            scientificName: scientificNameField.text,
            plantType: plantTypeField.text,
            lightRequirement: lightRequirementField.text,
            indoor: indoorField.text,
            floweringSeason: floweringSeasonField.text,
            wateringFrequency: wateringFrequencyField.text,
            wateringNotes: wateringNotesField.text,
            humidityPreference: humidityPreferenceField.text,
            soilType: soilTypeField.text,
            lastWatered: lastWateredField.text,
            fertilizingSchedule: fertilizingScheduleField.text,
            lastFertilized: lastFertilizedField.text,
            pruningTime: pruningTimeField.text,
            pruningNotes: pruningNotesField.text,
            lastPruned: lastPrunedField.text,
            growthRate: growthRateField.text,
            issuesPests: issuesPestsField.text,
            temperatureTolerance: temperatureToleranceField.text,
            toxicToPets: toxicToPetsField.text,
            poisonousToPets: poisonousToPetsField.text,
            poisonousToHumans: poisonousToHumansField.text,
            acquiredDate: acquiredDateField.text,
            source: sourceField.text,
            notes: notesField.text
        };

        let ok = false;
        if (root.isNewPlant) {
            const newId = plantListViewModel.addPlant(payload);
            ok = newId > 0;
        } else {
            ok = plantListViewModel.updatePlant(plantData.id, payload);
        }

        if (!ok) {
            statusLabel.text = plantListViewModel.lastError;
            return;
        }
        plantListViewModel.refresh();
        statusLabel.text = "";
        const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
        if (targetStack) {
            targetStack.pop();
        }
    }
}
