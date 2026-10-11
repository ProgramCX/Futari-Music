import QtQuick
import QtQuick.Layouts
import FutariMusic

Item {
    id: root
    property var characters: []
    property real positionMs: 0
    property int fontPixelSize: 23
    property bool bold: true
    property bool hovered: false
    implicitWidth: flow.implicitWidth
    implicitHeight: flow.implicitHeight

    Flow {
        id: flow
        width: root.width
        spacing: 0
        Repeater {
            model: root.characters
            delegate: Item {
                required property var modelData
                property real fillRatio: {
                    const start = Number(modelData.startMs)
                    const end = Number(modelData.endMs)
                    if (root.positionMs <= start) return 0
                    if (root.positionMs >= end) return 1
                    return (root.positionMs - start) / Math.max(1, end - start)
                }
                TextMetrics {
                    id: metrics
                    text: modelData.text
                    font.pixelSize: root.fontPixelSize
                    font.bold: root.bold
                }
                width: metrics.advanceWidth
                height: metrics.height
                Text {
                    anchors.left: parent.left
                    text: modelData.text
                    color: root.hovered ? Theme.muted : Theme.text
                    font.pixelSize: root.fontPixelSize
                    font.bold: root.bold
                }
                Item {
                    width: parent.width * parent.fillRatio
                    height: parent.height
                    clip: true
                    Text {
                        anchors.left: parent.left
                        text: modelData.text
                        color: Theme.accent
                        font.pixelSize: root.fontPixelSize
                        font.bold: root.bold
                    }
                }
            }
        }
    }
}
