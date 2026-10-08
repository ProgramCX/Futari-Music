import QtQuick
import QtQuick.Controls
import FutariMusic

ToolButton {
    id: control
    property string iconName: ""
    property color iconColor: Theme.iconPrimary
    property int iconSize: 19
    property color hoverColor: Theme.hover
    implicitWidth: 38
    implicitHeight: 38
    padding: 8
    scale: pressed ? 0.94 : 1
    Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.down ? Theme.buttonPressed : (control.hovered ? control.hoverColor : "transparent")
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
    }
    contentItem: SvgIcon {
        anchors.centerIn: parent
        name: control.iconName
        color: control.enabled ? control.iconColor : Theme.iconDisabled
        iconSize: control.iconSize
        visible: control.iconName.length > 0
    }
}
