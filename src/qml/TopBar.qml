import QtQuick 2.2
import "components"
import QtQuick.Layouts 1.1

Item {
    property real betweenSpace: (width - toolsButton.width * toolsButtonRow.children.length) / (toolsButtonRow.children.length + 1)
    width: parent.width
    height: 20

    RowLayout {
        id: toolsButtonRow
        x: betweenSpace
        anchors.verticalCenter: parent.verticalCenter
        spacing: betweenSpace

        TopButton {
            id: toolsButton
            hoverColor: "#ff7777"
            onClicked: colorPicker.visible = !colorPicker.visible
        }

        TopButton {
            hoverColor: "#abff6f"
            onClicked: brushSettings.visible = !brushSettings.visible
        }

        TopButton {
            hoverColor: "#6fb2ff"
            onClicked: console.log("magenta")
        }
    }
}