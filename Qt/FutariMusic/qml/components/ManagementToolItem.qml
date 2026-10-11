import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Button {
    id: control

    property string iconName: ""
    property string title: ""
    property string description: ""
    property string summaryText: ""

    implicitHeight: 96
    leftPadding: 16
    rightPadding: 18
    topPadding: 12
    bottomPadding: 12
    Accessible.name: title
    Accessible.description: description

    background: Rectangle {
        radius: Theme.radiusLarge
        color: !control.enabled
              ? Theme.buttonDisabledFill
              : control.down
                ? Theme.buttonPressed
                : control.hovered
                  ? Theme.hover
                  : Theme.surface
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.border
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
    }

    contentItem: RowLayout {
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            radius: Theme.radius
            color: Theme.secondary

            SvgIcon {
                anchors.centerIn: parent
                name: control.iconName
                color: control.enabled ? Theme.iconAccent : Theme.iconDisabled
                iconSize: 22
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: control.title
                color: control.enabled ? Theme.text : Theme.muted
                font.pixelSize: 15
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                text: control.description
                color: Theme.muted
                font.pixelSize: 13
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                visible: control.summaryText.length > 0
                text: control.summaryText
                color: Theme.muted
                font.pixelSize: 12
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        SvgIcon {
            name: "arrow-right"
            color: control.enabled && control.hovered ? Theme.iconAccent : Theme.iconSecondary
            iconSize: 18
            Layout.leftMargin: 8
        }
    }
}
