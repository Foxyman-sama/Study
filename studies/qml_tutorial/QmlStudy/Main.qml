import QtQuick

Rectangle {
    id: page
    width: 320; height: 480
    color: "lightgray"

    Text {
        id: hello_text
        text: "Hello, World!"
        y: 30
        anchors.horizontalCenter: page.horizontalCenter
        font.pointSize: 24; font.bold: true
    }
}
