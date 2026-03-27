import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var stackView: null
    property string editingTag: ""

    background: Rectangle { color: "#10171d" }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: qsTr("Back")
                onClicked: {
                    const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
                    if (targetStack) {
                        targetStack.pop()
                    }
                }
            }
            Label {
                text: qsTr("Tags")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: ui.point_size_medium
            }
            Item { width: 48; height: 1 }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ui.margin_large
        spacing: ui.spacing_medium

        Label {
            text: qsTr("Manage reusable plant tags.")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.8
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ui.spacing_medium

            TextField {
                id: newTagField
                Layout.fillWidth: true
                placeholderText: qsTr("New tag")
            }

            Button {
                text: qsTr("Add")
                onClicked: {
                    if (tagCatalogViewModel.addTag(newTagField.text)) {
                        newTagField.text = ""
                    }
                }
            }
        }

        Label {
            visible: tagCatalogViewModel.lastError.length > 0
            text: tagCatalogViewModel.lastError
            color: "tomato"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: tagCatalogViewModel
            spacing: ui.spacing_small

            delegate: Rectangle {
                required property string name
                width: ListView.view.width
                implicitHeight: 64
                radius: 12
                color: "#18222c"
                border.color: "#2b3946"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: ui.margin_medium
                    spacing: ui.spacing_medium

                    TextField {
                        id: editField
                        Layout.fillWidth: true
                        text: name
                        enabled: root.editingTag === name
                        readOnly: root.editingTag !== name
                    }

                    Button {
                        text: root.editingTag === name ? qsTr("Save") : qsTr("Rename")
                        onClicked: {
                            if (root.editingTag === name) {
                                if (tagCatalogViewModel.updateTag(name, editField.text)) {
                                    root.editingTag = ""
                                }
                            } else {
                                root.editingTag = name
                                editField.forceActiveFocus()
                                editField.selectAll()
                            }
                        }
                    }

                    Button {
                        text: qsTr("Remove")
                        onClicked: {
                            if (tagCatalogViewModel.removeTag(name) && root.editingTag === name) {
                                root.editingTag = ""
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { }
        }
    }
}
