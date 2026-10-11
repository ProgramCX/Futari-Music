import QtQuick
import QtQuick.Effects
import FutariMusic

Item {
    id: root
    property string name: "music-note"
    property color color: Theme.iconPrimary
    property int iconSize: 20
    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: sourceIcon
        anchors.fill: parent
        source: root.name.length > 0 ? Qt.resolvedUrl("../assets/icons/" + root.name + ".svg") : ""
        sourceSize.width: root.iconSize * 2
        sourceSize.height: root.iconSize * 2
        visible: false
        smooth: true
    }
    MultiEffect {
        anchors.fill: parent
        source: sourceIcon
        colorization: 1
        colorizationColor: root.color
    }
}
