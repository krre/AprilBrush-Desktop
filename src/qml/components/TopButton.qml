import QtQuick 2.2

Rectangle {
    id: root
    property color hoverColor: "blue"
    width: 100
    height: 10
    radius: 10
    color: mouseArea.containsMouse ? hoverColor : "lightgray"
    opacity: mouseArea.pressed ? 0.5 : 1

    signal clicked

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}