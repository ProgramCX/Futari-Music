import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Dialog {
    id: control
    popupType: Popup.Item
    modal: true
    padding: 22
    background: PopupSurface {}
    header: Item {
        implicitHeight: 60
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 22; anchors.rightMargin: 12
            Text { text: control.title; Layout.fillWidth: true; color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold; elide: Text.ElideRight }
            FutariToolButton { iconName: "window-close"; onClicked: control.reject(); ToolTip.visible: hovered; ToolTip.text: "关闭" }
        }
    }
    footer: DialogButtonBox {
        visible: control.standardButtons !== Dialog.NoButton
        standardButtons: control.standardButtons
        padding: 16
        spacing: 8
        background: Item {}
        delegate: FutariButton {}
    }
    Overlay.modal: Rectangle { color: Theme.modalScrim }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionFast } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motionFast } }
}
