import QtQuick
import QtQuick.Controls
import FutariMusic

Menu {
    popupType: Popup.Item
    implicitWidth: 240
    padding: 6
    margins: 10
    background: PopupSurface { radius: Theme.radius }
    delegate: FutariMenuItem {}
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionFast } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motionFast } }
}
