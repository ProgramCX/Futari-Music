import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Item {
    id: root
    property bool allowed: true
    signal singleRequested()
    signal batchRequested()
    implicitWidth: uploadButton.implicitWidth
    implicitHeight: uploadButton.implicitHeight

    function openMenu() {
        if (!root.allowed) return
        const point = uploadButton.mapToItem(Overlay.overlay, 0, uploadButton.height + 6)
        menu.x = Math.max(8, Math.min(point.x, Overlay.overlay.width - menu.width - 8))
        menu.y = Math.min(point.y, Overlay.overlay.height - menu.height - 8)
        closeTimer.stop()
        menu.open()
    }

    FutariButton {
        id: uploadButton
        anchors.fill: parent
        text: "上传歌曲"
        iconName: "upload"
        enabled: root.allowed
        onClicked: menu.visible ? menu.close() : root.openMenu()
        ToolTip.visible: hovered && !enabled
        ToolTip.text: "需要管理员身份或歌曲上传权限"
        HoverHandler {
            id: buttonHover
            onHoveredChanged: {
                if (hovered) root.openMenu()
                else closeTimer.restart()
            }
        }
    }

    Timer {
        id: closeTimer
        interval: 180
        onTriggered: if (!buttonHover.hovered && !menuHover.hovered) menu.close()
    }

    Popup {
        id: menu
        popupType: Popup.Item
        parent: Overlay.overlay
        width: 172
        height: 104
        padding: 6
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }
        background: PopupSurface { radius: Theme.radius }
        HoverHandler { id: menuHover; onHoveredChanged: if (hovered) closeTimer.stop(); else closeTimer.restart() }
        ColumnLayout {
            anchors.fill: parent
            spacing: 3
            FutariButton { Layout.fillWidth: true; text: "单曲上传"; iconName: "music-note"; onClicked: { menu.close(); root.singleRequested() } }
            FutariButton { Layout.fillWidth: true; text: "批量上传"; iconName: "queue"; onClicked: { menu.close(); root.batchRequested() } }
        }
    }
}
