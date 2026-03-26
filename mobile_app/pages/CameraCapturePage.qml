import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia
import QtCore
import "../../ui"

Page {
    id: root
    AppConstants { id: ui }
    property var stackView: null
    property var onCaptured: null
    property bool captureInProgress: false
    readonly property bool cameraReady: camera.active && imageCapture.readyForCapture

    function closePage() {
        const targetStack = root.stackView ? root.stackView : (StackView.view ? StackView.view : null)
        if (targetStack) {
            targetStack.pop()
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: ui.spacing_medium

            ToolButton {
                text: qsTr("Cancel")
                onClicked: closePage()
            }

            Label {
                text: qsTr("Take Photo")
                font.pixelSize: ui.point_size_medium
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Item { Layout.preferredWidth: implicitWidth }
        }
    }

    MediaDevices {
        id: mediaDevices
    }

    Camera {
        id: camera
        cameraDevice: mediaDevices.defaultVideoInput
        active: true
        onActiveChanged: {
            if (active) {
                statusLabel.text = imageCapture.readyForCapture
                                 ? ""
                                 : qsTr("Camera is warming up...")
            } else if (!statusLabel.text) {
                statusLabel.text = qsTr("Camera is not active.")
            }
        }
        onErrorOccurred: function(error, errorString) {
            statusLabel.text = errorString && errorString.length > 0
                             ? errorString
                             : qsTr("Failed to start camera.")
        }
    }

    ImageCapture {
        id: imageCapture
        onReadyForCaptureChanged: function(ready) {
            if (ready) {
                if (root.captureInProgress) {
                    statusLabel.text = qsTr("Saving photo...")
                } else {
                    statusLabel.text = ""
                }
            } else if (camera.active && !root.captureInProgress) {
                statusLabel.text = qsTr("Camera is warming up...")
            }
        }
        onImageExposed: function(requestId) {
            root.captureInProgress = true
            statusLabel.text = qsTr("Saving photo...")
        }
        onImageCaptured: function(requestId, preview) {
            root.captureInProgress = true
            statusLabel.text = qsTr("Saving photo...")
        }
        onImageSaved: function(requestId, filePath) {
            root.captureInProgress = false
            statusLabel.text = ""
            if (typeof root.onCaptured === "function") {
                root.onCaptured(filePath)
            }
            closePage()
        }
        onErrorOccurred: function(requestId, error, errorString) {
            root.captureInProgress = false
            statusLabel.text = errorString.length > 0 ? errorString : qsTr("Failed to capture image.")
        }
    }

    CaptureSession {
        camera: camera
        imageCapture: imageCapture
        videoOutput: videoOutput
    }

    Component.onCompleted: {
        statusLabel.text = qsTr("Starting camera...")
        camera.active = true
    }
    Component.onDestruction: camera.active = false

    ColumnLayout {
        anchors.fill: parent
        spacing: ui.spacing_medium

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#101010"
            radius: 8
            clip: true

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectCrop
            }
        }

        Label {
            id: statusLabel
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: "#ff6b6b"
        }

        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            RoundButton {
                text: qsTr("Capture")
                highlighted: true
                enabled: root.cameraReady && !root.captureInProgress
                onClicked: {
                    if (!root.cameraReady) {
                        statusLabel.text = qsTr("Camera is not ready yet.")
                        return
                    }
                    root.captureInProgress = true
                    statusLabel.text = qsTr("Capturing photo...")
                    const requestId = imageCapture.captureToFile()
                    if (requestId < 0) {
                        root.captureInProgress = false
                        statusLabel.text = qsTr("Could not start photo capture.")
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }
    }
}
