import QtQuick
import QtQuick.Controls
import FutariMusic

ComboBox {
    id: control
    implicitWidth: 136
    implicitHeight: 40
    leftPadding: 12
    rightPadding: 34
    font.pixelSize: 14

    contentItem: Text {
        leftPadding: control.leftPadding
        rightPadding: control.rightPadding
        text: control.displayText
        color: control.enabled ? Theme.text : Theme.muted
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: SvgIcon {
        x: control.width - width - 12
        y: (control.height - height) / 2
        name: "chevron-down"
        iconSize: 14
        color: control.enabled ? Theme.iconSecondary : Theme.iconDisabled
    }

    delegate: ItemDelegate {
        id: optionDelegate
        width: control.width
        height: 38
        highlighted: control.highlightedIndex === index
        contentItem: Text {
            text: control.textRole.length > 0 ? modelData[control.textRole] : modelData
            color: Theme.text
            font: control.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: optionDelegate.highlighted ? Theme.hover : "transparent"
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.enabled ? Theme.fieldFill : Theme.buttonDisabledFill
        border.width: control.visualFocus ? 1.5 : 1
        border.color: control.visualFocus ? Theme.fieldFocusBorder : Theme.fieldBorder
        Behavior on color { ColorAnimation { duration: Theme.colorTransition } }
    }

    popup: Popup {
        y: control.height + 6
        width: control.width
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, 230)

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }
    }
}
