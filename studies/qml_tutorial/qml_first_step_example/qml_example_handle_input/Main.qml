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

    focus: true

    Keys.onPressed: {
        if (event.key == Qt.Key_Return) {
            color = "white";
            event.accepted = true;
        }
    }
}
