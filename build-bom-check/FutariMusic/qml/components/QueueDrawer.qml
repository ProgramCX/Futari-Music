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
    property bool batchMode: false
    property var selectedSongIds: []
    readonly property var roomPlayback: appController.roomState.playback || ({})
    // 当前项要跟随真实播放源切换，房间状态与本地播放器互不覆盖。
    readonly property real currentSongId: root.roomQueue
        ? Number(root.roomPlayback.currentSongId || 0)
        : Number(appController.player.song.id || 0)
    readonly property bool currentSongPlaying: root.roomQueue
        ? root.roomPlayback.status === "playing"
        : appController.player.playing
    readonly property int currentSongIndex: root.songIds.indexOf(root.currentSongId)
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
    function toggleSelection(songId, checked) {
        // Assign a fresh array so QML reevaluates row selection bindings immediately.
        const ids = root.selectedSongIds.slice()
        const target = Number(songId)
        const index = ids.indexOf(target)
        if (checked && index < 0) ids.push(target)
        else if (!checked && index >= 0) ids.splice(index, 1)
        root.selectedSongIds = ids
    }
    function setAllSelected(checked) {
        root.selectedSongIds = checked ? root.songIds.map(songId => Number(songId)) : []
    }
    function revealCurrentSong() {
        if (!root.visible || root.currentSongIndex < 0) return
        queueList.positionViewAtIndex(root.currentSongIndex, ListView.Contain)
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
    onSongIdsChanged: selectedSongIds = selectedSongIds.filter(songId =>
        root.songIds.some(queueId => Number(queueId) === Number(songId)))
    onVisibleChanged: {
        if (visible) roomQueue = !!appController.roomState.room
        else { draggedSongId = 0; dropIndex = -1; batchMode = false; selectedSongIds = [] }
        if (visible) Qt.callLater(root.revealCurrentSong)
    }
    onCurrentSongIndexChanged: Qt.callLater(root.revealCurrentSong)
    Connections {
        target: appController.player
        function onPlayingChanged() { root.revealCurrentSong() }
        function onSongChanged() { root.revealCurrentSong() }
    }
    Connections {
        target: appController
        function onRoomStateChanged() {
            if (!root.editable) {
                root.batchMode = false
                root.selectedSongIds = []
            }
            if (!appController.roomState.room) root.roomQueue = false
        }
    }
    Rectangle { width: 1; height: parent.height; color: Theme.border }
    Column {
        anchors.fill: parent; anchors.margins: 20; spacing: 16
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "播放队列"
                color: Theme.text
                font.pixelSize: 21
                font.bold: true
                Layout.fillWidth: true
            }
            FutariToolButton {
                visible: root.editable
                iconName: root.batchMode ? "window-close" : "list-check"
                ToolTip.visible: hovered
                ToolTip.text: root.batchMode ? "退出多选" : "多选歌曲"
                onClicked: {
                    root.batchMode = !root.batchMode
                    if (!root.batchMode) root.selectedSongIds = []
                }
            }
        }
        Row {
            visible: root.showModePicker && !!appController.roomState.room; spacing: 6
            FutariButton { text: "房间"; prominent: root.roomQueue; onClicked: root.roomQueue = true }
            FutariButton { text: "本地"; prominent: !root.roomQueue; onClicked: root.roomQueue = false }
        }
        RowLayout {
            Layout.fillWidth: true
            Text { text: root.songIds.length + (root.roomQueue ? " 首歌曲 · 房间共享" : " 首歌曲 · 本地队列"); color: Theme.muted; font.pixelSize: 12; Layout.fillWidth: true }
            FutariCheckBox {
                visible: root.batchMode
                text: "全选"
                checked: root.songIds.length > 0 && root.songIds.every(songId => root.selectedSongIds.indexOf(Number(songId)) >= 0)
                onClicked: root.setAllSelected(checked)
            }
            FutariToolButton {
                visible: root.batchMode
                iconName: "trash"
                iconColor: Theme.danger
                enabled: root.selectedSongIds.length > 0
                ToolTip.visible: hovered
                ToolTip.text: "删除已选歌曲（" + root.selectedSongIds.length + "）"
                onClicked: {
                    appController.removeQueueSongs(root.selectedSongIds)
                    root.selectedSongIds = []
                }
            }
        }
        Text { visible: !root.roomQueue && !!appController.roomState.room; text: "本地队列保留，离开房间后继续使用"; color: Theme.muted; font.pixelSize: 11; width: parent.width; wrapMode: Text.Wrap }
        ListView {
            id: queueList
            width: parent.width; height: Math.max(0, parent.height - y); clip: true; spacing: 6
            model: root.songIds
            onCountChanged: Qt.callLater(root.revealCurrentSong)
            delegate: Rectangle {
                id: queueRow
                property var queueSong: { appController.songs; return appController.songForId(modelData) }
                readonly property bool current: Number(modelData) === root.currentSongId
                width: ListView.view.width; height: 56; radius: 10
                color: root.selectedSongIds.indexOf(Number(modelData)) >= 0 || current
                    ? Theme.secondary : (queueHover.hovered ? Theme.hover : "transparent")
                antialiasing: true
                HoverHandler { id: queueHover }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; spacing: 3
                    FutariCheckBox {
                        visible: root.batchMode
                        Layout.preferredWidth: 24
                        text: ""
                        checked: root.selectedSongIds.indexOf(Number(modelData)) >= 0
                        onClicked: root.toggleSelection(modelData, checked)
                    }
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
                    Item {
                        Layout.preferredWidth: 40; Layout.preferredHeight: 40
                        TapHandler {
                            enabled: root.editable
                            onTapped: queueRow.current
                                ? appController.togglePlayback()
                                : appController.playSong(queueSong)
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: 7
                            color: Theme.surface
                            clip: true
                            SvgIcon {
                                anchors.centerIn: parent
                                name: "music-note"
                                iconSize: 19
                                color: Theme.iconAccent
                            }
                            Image {
                                anchors.fill: parent
                                source: appController.covers[String(queueSong.id || modelData)] || ""
                                fillMode: Image.PreserveAspectCrop
                                visible: status === Image.Ready
                            }
                            Rectangle {
                                anchors.fill: parent
                                color: "#66000000"
                                visible: queueRow.current && root.currentSongPlaying
                            }
                            SvgIcon {
                                anchors.centerIn: parent
                                name: queueRow.current && root.currentSongPlaying ? "pause" : "play"
                                iconSize: 18
                                color: "white"
                                visible: queueRow.current || queueHover.hovered
                            }
                        }
                    }
                    ColumnLayout { Layout.fillWidth: true; spacing: 1
                        Text {
                            text: queueSong.title || "歌曲 #" + modelData
                            color: queueRow.current ? Theme.accent : Theme.text
                            font.weight: queueRow.current ? Font.DemiBold : Font.Normal
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text { text: queueSong.artist || ""; color: Theme.muted; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                    }
                    FutariToolButton {
                        iconName: queueRow.current ? (root.currentSongPlaying ? "pause" : "play") : "play"
                        visible: root.editable && !root.batchMode
                        opacity: queueHover.hovered || queueRow.current ? 1 : 0
                        enabled: visible && (queueHover.hovered || queueRow.current)
                        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                        onClicked: queueRow.current ? appController.togglePlayback() : appController.playSong(queueSong)
                    }
                    FutariToolButton {
                        iconName: "remove"
                        visible: root.editable && !root.batchMode
                        opacity: queueHover.hovered || queueRow.current ? 1 : 0
                        enabled: visible && (queueHover.hovered || queueRow.current)
                        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                        onClicked: appController.removeQueueSong(index)
                    }
                }
                Rectangle { width: parent.width; height: 2; color: Theme.accent; visible: root.draggedSongId !== 0 && root.dropIndex === index }
            }
            footer: Rectangle { width: queueList.width; height: 2; color: Theme.accent; visible: root.draggedSongId !== 0 && root.dropIndex === root.songIds.length }
        }
    }
}
