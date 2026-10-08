import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Rectangle {
    id: root
    width: 335
    property bool roomQueue: !!appController.roomState.room
    property bool showModePicker: true
    color: Theme.surface
    property var songIds: roomQueue ? (appController.roomState.songIds || []) : appController.localQueue
    readonly property bool editable: roomQueue ? appController.canControl : !appController.roomState.room
    onVisibleChanged: if (visible) roomQueue = !!appController.roomState.room
    Connections { target: appController; function onRoomStateChanged() { if (!appController.roomState.room) root.roomQueue = false } }
    Rectangle { width: 1; height: parent.height; color: Theme.border }
    Column {
        anchors.fill: parent; anchors.margins: 20; spacing: 16
        Text { text: "播放队列"; color: Theme.text; font.pixelSize: 21; font.bold: true }
        Row {
            visible: root.showModePicker && !!appController.roomState.room; spacing: 6
            FutariButton { text: "房间"; prominent: root.roomQueue; onClicked: root.roomQueue = true }
            FutariButton { text: "本地"; prominent: !root.roomQueue; onClicked: root.roomQueue = false }
        }
        Text { text: root.songIds.length + (root.roomQueue ? " 首歌曲 · 房间共享" : " 首歌曲 · 本地队列"); color: Theme.muted; font.pixelSize: 12 }
        Text { visible: !root.roomQueue && !!appController.roomState.room; text: "本地队列保留，离开房间后继续使用"; color: Theme.muted; font.pixelSize: 11; width: parent.width; wrapMode: Text.Wrap }
        ListView {
            width: parent.width; height: Math.max(0, parent.height - y); clip: true; spacing: 6
            model: root.songIds
            delegate: Rectangle {
                property var queueSong: { appController.songs; return appController.songForId(modelData) }
                width: ListView.view.width; height: 48; radius: 10; color: index % 2 ? "transparent" : Theme.secondary
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; spacing: 3
                    ColumnLayout { Layout.fillWidth: true; spacing: 1
                        Text { text: queueSong.title || "歌曲 #" + modelData; color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: queueSong.artist || ""; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                    }
                    FutariToolButton { iconName: "play"; visible: root.editable; onClicked: appController.playSong(queueSong) }
                    FutariToolButton { iconName: "queue-up"; visible: root.editable; enabled: index > 0; onClicked: appController.moveQueueSong(index, -1) }
                    FutariToolButton { iconName: "queue-down"; visible: root.editable; enabled: index < root.songIds.length - 1; onClicked: appController.moveQueueSong(index, 1) }
                    FutariToolButton { iconName: "remove"; visible: root.editable; onClicked: appController.removeQueueSong(index) }
                }
            }
        }
    }
}
