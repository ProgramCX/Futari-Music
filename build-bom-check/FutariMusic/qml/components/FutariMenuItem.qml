import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

MenuItem {
    id: control
    property string iconName: ""
    implicitHeight: visible ? 42 : 0
    implicitWidth: 228
    leftPadding: 12
    rightPadding: 12
    contentItem: RowLayout {
        spacing: 10
        SvgIcon { visible: control.iconName.length > 0; name: control.iconName; iconSize: 18; color: control.enabled ? Theme.iconPrimary : Theme.iconDisabled }
        Text { text: control.text; Layout.fillWidth: true; color: control.enabled ? Theme.text : Theme.muted; font.pixelSize: 14; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
    }
    background: Rectangle {
        radius: Theme.radiusSmall
        antialiasing: true
        color: control.down ? Theme.buttonPressed : (control.highlighted || control.hovered ? Theme.hover : "transparent")
    }
}
