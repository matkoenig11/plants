import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


ColumnLayout {
    Label {
        id: generalCareLabel
        text: qsTr("General Information")
        font.pixelSize: 20
        Layout.margins: 8
    }

    Label {
        text: qsTr("Light Requirement " )
        font.pixelSize: 16
        Layout.margins: 8
    }
}