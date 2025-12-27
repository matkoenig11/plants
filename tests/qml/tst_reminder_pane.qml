import QtQuick 2.15
import QtQuick.Controls 2.15
import QtTest 1.2
import "../../app/components"

TestCase {
    name: "ReminderPane"

    ReminderPane {
        id: pane
        reminderModel: QtObject {}
        plantModel: QtObject {}
        settingsModel: QtObject {}
    }

    function test_defaults() {
        verify(pane !== null)
    }
}
