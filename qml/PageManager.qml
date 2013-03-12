import QtQuick 2.0
import "utils.js" as Utils

Rectangle {
    property int countPage: 2

    width: 600
    height: 40
    color: "#eeeeee"
    border.color: "gray"
    radius: 7
    antialiasing: true

    Row {
        anchors.fill: parent
        // Add page button
        Item {
            id: addPageArea
            width: 30
            height: parent.height
            Text {
                text: "+"
                font.bold: true
                font.pixelSize: 15
                anchors.centerIn: parent
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    var numNextPage = Utils.zeroFill(++countPage, 2)
                    pagesModel.append({name: "Page-" + numNextPage})
                }

            }
        }

        ListView {
            id: pagesView
            model: pagesModel
            delegate: pagesDelegate
            highlight: pageSelected

            height: parent.height - 8
            width: parent.width - addPageArea.width - 10
            anchors.verticalCenter: parent.verticalCenter
            orientation: ListView.Horizontal
            spacing: 5
            clip: true
        }

        ListModel {
            id: pagesModel
            ListElement { name: "Page-01" }
            ListElement { name: "Page-02" }
        }

        Component {
            id: pagesDelegate

            Rectangle {
                id: pageContaner
                width: 100
                height: pagesView.height
                color: "lightgray"
                border.width: 1
                border.color: "gray"
                radius: 5
                antialiasing: true

                Text {
                    text: name
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: pagesView.currentIndex = index
                }

                Item {
                    width: 15
                    height: 15
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    Text {
                        text: "x"
                        anchors.centerIn: parent
                        font.pointSize: 10
                        font.bold: closeMouseArea.containsMouse ? true : false
                    }
                    MouseArea {
                        id: closeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (pagesModel.count > 0) pagesModel.remove(index)
                        }
                    }
                }
            }
        }

        Component {
            id: pageSelected
            Rectangle {
                width: 100
                height: pagesView.height
                color: "red"
            }
        }
    }
}