import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Button {
    id: control
    property string iconName: ""
    property int iconSize: 18
    property bool prominent: false
    property color accent: Theme.accent
    implicitHeight: 40
    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    scale: pressed ? 0.985 : 1
    Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    background: Rectangle {
        radius: Theme.radius
        color: !control.enabled ? Theme.buttonDisabledFill : control.prominent
              ? (control.down ? Qt.darker(control.accent, 1.12) : control.accent)
              : (control.down ? Theme.buttonPressed : (control.hovered ? Theme.buttonHover : Theme.buttonFill))
        border.width: control.visualFocus ? 2 : 0
        border.color: control.accent
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
    }
    contentItem: RowLayout {
        spacing: 8
        SvgIcon {
            visible: control.iconName.length > 0
            name: control.iconName
            color: !control.enabled ? Theme.iconDisabled : (control.prominent ? Theme.iconOnAccent : Theme.iconPrimary)
            iconSize: control.iconSize
            Layout.alignment: control.text.length === 0 ? Qt.AlignHCenter : Qt.AlignVCenter
        }
        Text {
            text: control.text
            visible: text.length > 0
            color: !control.enabled ? Theme.buttonDisabledText : (control.prominent ? Theme.buttonProminentText : Theme.buttonText)
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
