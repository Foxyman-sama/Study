import QtQuick

Rectangle {
    width: 640
    height: 480
    color: "red"

    Text {
        anchors.centerIn: parent
        text: "Hello, World!"
    }

    TapHandler {
        onTapped: parent.color = "blue"
    }
}
