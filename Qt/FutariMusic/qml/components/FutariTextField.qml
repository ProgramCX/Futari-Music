import QtQuick
import QtQuick.Controls
import FutariMusic

TextField {
    id: control
    property bool authStyle: false
    implicitHeight: 42
    leftPadding: 14
    rightPadding: 14
    color: control.authStyle ? Theme.authText : Theme.text
    placeholderTextColor: control.authStyle ? Theme.authPlaceholder : Theme.muted
    selectionColor: Theme.accent
    selectedTextColor: "white"

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.authStyle ? Theme.authField : Theme.fieldFill
        border.width: control.activeFocus ? 1.5 : 1
        border.color: control.activeFocus
                     ? (control.authStyle ? Theme.authAccent : Theme.fieldFocusBorder)
                     : (control.authStyle ? Theme.authBorder : Theme.fieldBorder)
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
        Behavior on border.color { ColorAnimation { duration: Theme.colorTransition } }
    }
}
