/* AprilBrush - Digital Painting Software
 * Copyright (C) 2012-2014 Vladimir Zarypov <krre31@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import ABLib 1.0
import "components"
import "undo.js" as Undo
import "utils.js" as Utils

Item {
    id: root
    property alias canvas: canvasView.currentItem
    property alias canvasView: canvasView
    property color bgColor: "white"
    property bool isPan: false
    property bool isPick: false
    property point grabPoint

    property real zoom: 1.0
    property int mirror: 1
    property real rotation: 0

    onBgColorChanged: layerModel.get(layerModel.count - 1).canvas.clear(bgColor)
    onIsPanChanged: coreLib.setCursorShape(isPan ? "OpenHand" : "Paint", brushSettings.size * zoom)
    onZoomChanged: coreLib.setCursorShape(isPan ? "OpenHand" : "Paint", brushSettings.size * zoom)

    Keys.onPressed: {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            grabPoint = Qt.point(mouseArea.mouseX, mouseArea.mouseY)
            isPan = true
        }
        if (event.modifiers & Qt.ControlModifier) { isPick = true }
    }

    Keys.onReleased: {
        if (Qt.ControlModifier) { isPick = false }
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) { isPan = false }
    }

    transform: [
        Scale { origin.x: width / 2; origin.y: height / 2; xScale: zoom * mirror; yScale: zoom },
        Rotation { origin.x: width / 2; origin.y: height / 2; angle: rotation }
    ]

    Component.onCompleted: {
        forceActiveFocus()
    }

    function resetTransform() {
        zoom = 1
        mirror = 1
        rotation = 0
        content.x = (width - content.width) / 2
        content.y = (height - content.height) / 2
    }

    Item {
        id: content
        width: imageSize.width
        height: imageSize.height

        CheckerBoard {
            anchors.fill: parent
            cellSide: 30
        }

        // temporary dirty hack for undo eraser brush
        Canvas {
            id: undoEraserBuffer
            anchors.fill: parent
            visible: false
            smooth: false
        }

        Canvas {
            id: buffer
            z: 1001
            anchors.fill: parent
            parent: canvasView.currentItem
            opacity: brushSettings.opacity / 100
            smooth: false

            MouseArea {
                id: mouseArea
                property real deltaDab: Math.max(brushSettings.spacing / 100 * brushSettings.size, 1)
                property var points: []
                property bool linearMode: false
                property point lastDrawPoint
                property point startPos
                property point finalPos
                anchors.fill: parent
                enabled: buffer.parent ? buffer.parent.enabled : false
                hoverEnabled: true

                function bezierCurve(start, control, end, t) {
                    var x, y
                    // linear bezier curve
                    if (!control) {
                        x = (1 - t) * start.x + t * end.x
                        y = (1 - t) * start.y + t * end.y
                    }
                    // quad bezier curve
                    else {
                        x = Math.pow((1 - t), 2) * start.x + 2 * t * (1 - t) * control.x + t * t * end.x
                        y = Math.pow((1 - t), 2) * start.y + 2 * t * (1 - t) * control.y + t * t * end.y
                    }

                    return Qt.point(x, y)
                }

                onHoveredChanged: coreLib.setCursorShape(containsMouse ? "Paint" : "Arrow", brushSettings.size * zoom)

                onPressed: {
                    var point = Qt.point(mouseX, mouseY)
                    if (isPick) {
                        coreLib.setCursorShape("PickColor", 0)
                        Utils.pickColor(point)
                    } else if (!isPan) {
                        if (isEraser) {
                            var undoEraserCtx = undoEraserBuffer.getContext("2d")
                            undoEraserCtx.clearRect(0, 0, width, height)
                            undoEraserCtx.drawImage(canvas, 0, 0)
                            undoEraserBuffer.requestPaint()
                        }

                        startPos = Qt.point(point.x, point.y)
                        finalPos = Qt.point(point.x, point.y)
                        lastDrawPoint = point
                        drawDab(point)
                        points = []
                        points.push(point)
                    }
                }

                onReleased: {
                    mainRoot.pressure = 1
                    if (isPan) {
                        coreLib.setCursorShape("OpenHand", 0)
                    } else if (isPick) {
                        coreLib.setCursorShape("Paint", brushSettings.size * zoom)
                    } else {
                        startPos.x -= dab.width
                        startPos.y -= dab.width
                        finalPos.x += dab.width
                        finalPos.y += dab.width

                        var bufferCtx = parent.getContext("2d")
                        var canvasCtx = canvas.getContext("2d")
                        if (!isEraser) {
                            var undoArea = canvasCtx.getImageData(startPos.x, startPos.y, finalPos.x - startPos.x, finalPos.y - startPos.y)
                            var redoArea = bufferCtx.getImageData(startPos.x, startPos.y, finalPos.x - startPos.x, finalPos.y - startPos.y)
                        } else {
                            undoArea = undoEraserBuffer.getContext("2d").getImageData(startPos.x, startPos.y, finalPos.x - startPos.x, finalPos.y - startPos.y)
                            redoArea = canvasCtx.getImageData(startPos.x, startPos.y, finalPos.x - startPos.x, finalPos.y - startPos.y)
                        }
                        undoManager.add(Undo.paint(startPos, undoArea, redoArea, brushSettings.opacity / 100, isEraser))
                        bufferCtx.clearRect(0, 0, width, height)
                        parent.requestPaint()
                    }
                }

                onPositionChanged: {
                    if (isPan) {
                        content.x += (mouseX - grabPoint.x)
                        content.y += (mouseY - grabPoint.y)
                    }
                    if (!pressed) { return; }
                    if (isPick) {
                        Utils.pickColor(Qt.point(mouseX, mouseY))
                    } else if (!isPan) {
                        var currentPoint = Qt.point(mouseX, mouseY)
                        var startPoint = lastDrawPoint
                        var currentSpacing = Math.sqrt(Math.pow(currentPoint.x - startPoint.x, 2) + Math.pow(currentPoint.y - startPoint.y, 2))
                        var numDabs = Math.floor(currentSpacing / deltaDab)
                        if (numDabs >= 1) {

                            if (points.length == 1 || numDabs < 3 || linearMode) {
                                var endPoint = currentPoint
                            } else {
                                var controlPoint = points[points.length - 1]
                                endPoint = Qt.point((controlPoint.x + currentPoint.x) / 2, (controlPoint.y + currentPoint.y) / 2)
                            }

                            var deltaT = 1 / numDabs
                            var betweenPoint = startPoint
                            var t = deltaT
                            var diff
                            while (t > 0 && t <= 1) {
                                var point = bezierCurve(startPoint, controlPoint, endPoint, t)
                                var deltaPoint = Math.sqrt(Math.pow(point.x - betweenPoint.x, 2) + Math.pow(point.y - betweenPoint.y, 2))
                                // check on bezier loop
                                if (diff && Math.abs(deltaPoint - deltaDab) > Math.abs(diff)) { break; }
                                diff = deltaPoint - deltaDab
                                if (Math.abs(diff <= 0.5)) {
                                    drawDab(point)
                                    diff = undefined
                                    betweenPoint = point
                                    t += deltaT
                                } else {
                                    t -= diff / deltaDab * deltaT
                                }
                            }
                            points.push(currentPoint)
                            lastDrawPoint = betweenPoint
                        }
                    }
                }

                function drawDab(point) {
                    var ctx = isEraser ? canvas.getContext("2d") : buffer.getContext("2d")
                    ctx.save()
                    ctx.globalCompositeOperation = isEraser ? "destination-out" : "source-over"
                    ctx.globalAlpha = mainRoot.pressure
                    var size = brushSettings.size
                    var x = point.x - size / 2 + size * brushSettings.jitter / 100 * (1 - 2 * Math.random())
                    var y = point.y - size / 2 + size * brushSettings.jitter / 100 * (1 - 2 * Math.random())

                    if (x < startPos.x) { startPos.x = Math.min(0, x) }
                    if (y < startPos.y) { startPos.y = Math.min(0, y) }
                    if (x > finalPos.x) { finalPos.x = Math.max(x, imageSize.width) }
                    if (y > finalPos.y) { finalPos.y = Math.max(y, imageSize.height) }

                    ctx.drawImage(dab, x, y)
                    ctx.restore()
                    if (isEraser) {
                        canvas.markDirty(x, y, dab.width, dab.height)
                    } else {
                        buffer.markDirty(x, y, dab.width, dab.height)
                    }
                }
            }
        }

        ListView {
            id: canvasView
            anchors.fill: parent
            model: layerModel
            spacing: -width
            orientation: ListView.Horizontal
            currentIndex: layerManager.layerView.currentIndex
            interactive: false
            signal created(var index, var canvas)
            delegate: Canvas {
                width: ListView.view.width
                height: ListView.view.height
                z: 1000 - index
                visible: isVisible
                enabled: !isLock
                smooth: false

                signal ready

                onAvailableChanged: {
                    clear(isBackground ? bgColor : null)
                    ready()
                }

                Component.onCompleted: {
                    layerModel.set(index, { "canvas": this })
                    canvasView.created(index, this)
                }

                function clear(color) {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (color) {
                        ctx.fillStyle = color
                        ctx.fillRect(0, 0, width, height)
                    }
                    requestPaint()
                }
            }
        }
/*
        CanvasItem {
            anchors.fill: parent
            Component.onCompleted: clear("green")
        }
        */
    }
}
