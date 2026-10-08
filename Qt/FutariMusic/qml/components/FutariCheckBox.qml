import QtQuick
import QtQuick.Controls
import FutariMusic

CheckBox {
    id: control
    spacing: 10
    leftPadding: 0
    rightPadding: 0
    implicitHeight: 36

    indicator: Rectangle {
        x: 0
        y: (control.height - height) / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 6
        color: control.checked
               ? (control.enabled ? Theme.accent : Theme.checkDisabledFill)
               : (control.enabled ? Theme.checkFill : Theme.checkDisabledFill)
        border.width: control.checked ? 0 : 1
        border.color: control.enabled
                     ? (control.hovered ? Theme.checkBorderHover : Theme.border)
                     : Theme.checkDisabledFill

        SvgIcon {
            anchors.centerIn: parent
            name: "check"
            iconSize: 14
            color: control.enabled ? Theme.iconOnAccent : Theme.checkDisabledMark
            visible: control.checked
        }
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
        Behavior on border.color { ColorAnimation { duration: Theme.colorTransition } }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        color: control.enabled ? Theme.text : Theme.muted
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
