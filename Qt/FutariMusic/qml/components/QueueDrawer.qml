import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Rectangle {
    id: root
    objectName: "queueDrawer"
    width: 335
    property bool roomQueue: !!appController.roomState.room
    property bool showModePicker: true
    // 拖动只维护插入位置预览；松手后控制器更新真正队列。
    property double draggedSongId: 0
    property int dropIndex: -1
    property real dragPointerY: 0
    function updateDrop(y) {
        dragPointerY = y
        const contentY = y + queueList.contentY
        let row = queueList.indexAt(1, contentY)
        // 行间距也归入紧接着的插入位置，不能误判为拖到队尾。
        if (row < 0 && y >= 0) row = queueList.indexAt(1, contentY + queueList.spacing)
        dropIndex = row < 0 ? (y < 0 ? 0 : songIds.length) : row
    }
    function finishDrag() {
        const before = dropIndex >= 0 && dropIndex < songIds.length ? songIds[dropIndex] : 0
        if (draggedSongId && editable && dropIndex >= 0)
            appController.moveQueueBefore(draggedSongId, before)
        draggedSongId = 0; dropIndex = -1
    }
    Timer {
        interval: 40; repeat: true; running: root.draggedSongId !== 0
        onTriggered: {
            const step = root.dragPointerY < 32 ? -12 : root.dragPointerY > queueList.height - 32 ? 12 : 0
            queueList.contentY = Math.max(0, Math.min(Math.max(0, queueList.contentHeight - queueList.height), queueList.contentY + step))
            root.updateDrop(root.dragPointerY)
        }
    }
    color: Theme.surface
    property var songIds: roomQueue ? (appController.roomState.songIds || []) : appController.localQueue
    readonly property bool editable: roomQueue ? appController.canControl : !appController.roomState.room
    onVisibleChanged: {
        if (visible) roomQueue = !!appController.roomState.room
        else { draggedSongId = 0; dropIndex = -1 }
    }
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
            id: queueList
            width: parent.width; height: Math.max(0, parent.height - y); clip: true; spacing: 6
            model: root.songIds
            delegate: Rectangle {
                id: queueRow
                property var queueSong: { appController.songs; return appController.songForId(modelData) }
                width: ListView.view.width; height: 48; radius: 10; color: index % 2 ? "transparent" : Theme.secondary
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; spacing: 3
                    Item {
                        objectName: "queueDragHandle" + index
                        visible: root.editable
                        Layout.preferredWidth: 24; Layout.fillHeight: true
                        SvgIcon { anchors.centerIn: parent; name: "drag-handle"; iconSize: 18; color: Theme.iconSecondary }
                        MouseArea {
                            anchors.fill: parent; cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            preventStealing: true
                            onPressed: mouse => {
                                root.draggedSongId = modelData
                                root.updateDrop(mapToItem(queueList, mouse.x, mouse.y).y)
                            }
                            onPositionChanged: mouse => {
                                if (pressed) root.updateDrop(mapToItem(queueList, mouse.x, mouse.y).y)
                            }
                            onReleased: root.finishDrag()
                            onCanceled: { root.draggedSongId = 0; root.dropIndex = -1 }
                        }
                    }
                    ColumnLayout { Layout.fillWidth: true; spacing: 1
                        Text { text: queueSong.title || "歌曲 #" + modelData; color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: queueSong.artist || ""; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                    }
                    FutariToolButton { iconName: "play"; visible: root.editable; onClicked: appController.playSong(queueSong) }
                    FutariToolButton { iconName: "queue-up"; visible: root.editable; enabled: index > 0; onClicked: appController.moveQueueSong(index, -1) }
                    FutariToolButton { iconName: "queue-down"; visible: root.editable; enabled: index < root.songIds.length - 1; onClicked: appController.moveQueueSong(index, 1) }
                    FutariToolButton { iconName: "remove"; visible: root.editable; onClicked: appController.removeQueueSong(index) }
                }
                Rectangle { width: parent.width; height: 2; color: Theme.accent; visible: root.draggedSongId !== 0 && root.dropIndex === index }
            }
            footer: Rectangle { width: queueList.width; height: 2; color: Theme.accent; visible: root.draggedSongId !== 0 && root.dropIndex === root.songIds.length }
        }
    }
}
