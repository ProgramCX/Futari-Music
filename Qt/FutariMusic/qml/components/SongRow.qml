import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Rectangle {
    id: root
    property var song: ({})
    property bool selected: false
    readonly property bool inRoom: !!appController.roomState.room
    readonly property bool actionsVisible: hover.hovered || activeFocus || actionScope.activeFocus || addMenu.visible || moreMenu.visible
    readonly property bool favorite: appController.favoriteSongIds.some(id => String(id) === String(root.song.id))
    signal playRequested()
    signal queueRequested()
    signal editRequested(var song)
    width: parent ? parent.width : 500
    height: 72
    radius: Theme.radiusSmall
    antialiasing: true
    activeFocusOnTab: true
    color: selected ? Theme.secondary : (hover.hovered ? Theme.hover : "transparent")
    HoverHandler { id: hover }
    Keys.onReturnPressed: if (!root.inRoom || appController.canControl) root.playRequested()

    function openMenu(menu, anchor) {
        const point = anchor.mapToItem(Overlay.overlay, 0, anchor.height + 5)
        menu.popup(Math.max(8, Math.min(point.x, Overlay.overlay.width - menu.width - 8)),
                   Math.max(8, Math.min(point.y, Overlay.overlay.height - menu.height - 8)))
    }
    function requestNewPlaylist() { newPlaylistName.text = ""; createPlaylistDialog.open() }

    RowLayout {
        anchors.fill: parent; anchors.margins: 10; spacing: 12
        Rectangle {
            Layout.preferredWidth: 50; Layout.preferredHeight: 50; radius: 9; color: Theme.secondary
            SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 23 }
            Image { anchors.fill: parent; source: appController.covers[String(root.song.id)] || ""; fillMode: Image.PreserveAspectCrop; visible: status === Image.Ready }
            FutariToolButton {
                anchors.centerIn: parent; visible: root.actionsVisible
                iconName: "play"; iconColor: "white"; hoverColor: "#80000000"
                enabled: !root.inRoom || appController.canControl
                background: Rectangle { radius: width / 2; color: "#60000000" }
                onClicked: root.playRequested(); ToolTip.visible: hovered; ToolTip.text: root.inRoom ? "在房间播放" : "播放"
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.minimumWidth: 0; spacing: 4
            Text { text: root.song.title || "未命名歌曲"; color: root.selected ? Theme.accent : Theme.text; font.pixelSize: 15; font.weight: root.selected ? Font.DemiBold : Font.Normal; elide: Text.ElideRight; Layout.fillWidth: true }
            Text { text: root.song.artist || "未知歌手"; color: Theme.muted; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true }
        }
        FocusScope {
            id: actionScope
            Layout.preferredWidth: 152; Layout.preferredHeight: 38
            Row {
                id: actionButtons; anchors.fill: parent
                opacity: root.actionsVisible ? 1 : 0
                enabled: root.actionsVisible
                Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                FutariToolButton {
                    iconName: root.favorite ? "heart-filled" : "heart"
                    iconColor: root.favorite ? Theme.danger : Theme.iconSecondary
                    enabled: !appController.favoriteBusy
                    onClicked: appController.toggleFavorite(root.song.id)
                    ToolTip.visible: hovered; ToolTip.text: root.favorite ? "取消喜欢" : "喜欢"
                }
                FutariToolButton { iconName: "download"; iconColor: Theme.iconSecondary; onClicked: appController.enqueueDownload(root.song); ToolTip.visible: hovered; ToolTip.text: "下载歌曲" }
                FutariToolButton { id: addButton; iconName: "plus"; iconColor: addMenu.visible ? Theme.accent : Theme.iconSecondary; onClicked: root.openMenu(addMenu, addButton); ToolTip.visible: hovered; ToolTip.text: "添加到" }
                FutariToolButton { id: moreButton; iconName: "more"; iconColor: moreMenu.visible ? Theme.accent : Theme.iconSecondary; onClicked: root.openMenu(moreMenu, moreButton); ToolTip.visible: hovered; ToolTip.text: "更多操作" }
            }
        }
        Text { visible: root.width > 650; Layout.preferredWidth: Math.min(190, root.width * 0.20); text: root.song.album || "未关联专辑"; color: Theme.muted; elide: Text.ElideRight; font.pixelSize: 12 }
        Text {
            Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight; color: Theme.muted; font.pixelSize: 12
            text: { const seconds = Math.floor(Number(root.song.durationMs || 0) / 1000); return seconds ? Math.floor(seconds / 60) + ":" + String(seconds % 60).padStart(2, "0") : "" }
        }
    }
    FutariMenu {
        id: addMenu; objectName: "songAddMenu"; parent: Overlay.overlay
        FutariMenuItem { text: root.inRoom && appController.canControl ? "房间共享队列（默认）" : "本地播放队列（默认）"; iconName: "queue"; onTriggered: root.queueRequested() }
        FutariMenuItem { visible: root.inRoom && appController.canControl; text: "本地播放队列"; iconName: "queue"; onTriggered: appController.addToLocalQueue(root.song.id) }
        FutariMenuItem { visible: !root.inRoom || !appController.canControl; text: "房间共享队列"; iconName: "room"; enabled: root.inRoom && appController.canControl; onTriggered: appController.addToRoomQueue(root.song.id) }
        FutariMenuSeparator {}
        FutariMenuItem { text: "创建新歌单并添加"; iconName: "plus"; onTriggered: root.requestNewPlaylist() }
        FutariMenuSeparator {}
        Repeater { model: appController.playlists; delegate: FutariMenuItem { text: modelData.name; iconName: "playlist"; onTriggered: appController.addSongToPlaylist(modelData.id, root.song.id) } }
        FutariMenuItem { visible: appController.playlists.length === 0; text: "还没有个人歌单"; enabled: false }
        FutariMenuSeparator { visible: appController.canManageServerPlaylist && appController.serverPlaylists.length > 0 }
        Repeater { model: appController.canManageServerPlaylist ? appController.serverPlaylists : []; delegate: FutariMenuItem { text: "公共 · " + modelData.name; iconName: "playlist"; onTriggered: appController.addSongToServerPlaylist(modelData.id, root.song.id) } }
    }
    FutariMenu {
        id: moreMenu; objectName: "songMoreMenu"; parent: Overlay.overlay
        FutariMenuItem { text: root.inRoom ? "在房间播放" : "播放"; iconName: "play"; enabled: !root.inRoom || appController.canControl; onTriggered: root.playRequested() }
        FutariMenuItem { text: "下一首播放"; iconName: "play-next"; enabled: !root.inRoom || appController.canControl; onTriggered: appController.playNext(root.song) }
        FutariMenuSeparator {}
        FutariMenuItem { text: "添加到…"; iconName: "plus"; onTriggered: root.openMenu(addMenu, addButton) }
        FutariMenuItem { text: "下载歌曲"; iconName: "download"; onTriggered: appController.enqueueDownload(root.song) }
        FutariMenuItem { text: root.favorite ? "取消喜欢" : "喜欢"; iconName: "heart"; enabled: !appController.favoriteBusy; onTriggered: appController.toggleFavorite(root.song.id) }
        FutariMenuSeparator { visible: appController.admin }
        FutariMenuItem { visible: appController.admin; text: "编辑歌曲"; iconName: "more"; onTriggered: root.editRequested(root.song) }
    }
    FutariDialog {
        id: createPlaylistDialog; title: "创建新歌单"; parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(420, parent.width - 32)
        ColumnLayout {
            width: parent.width; spacing: 16
            Text { text: "创建后将《" + (root.song.title || "这首歌") + "》加入歌单"; color: Theme.muted; Layout.fillWidth: true; wrapMode: Text.Wrap }
            FutariTextField { id: newPlaylistName; placeholderText: "歌单名称"; maximumLength: 100; Layout.fillWidth: true; enabled: !appController.playlistCreationBusy }
            RowLayout {
                Layout.fillWidth: true; Item { Layout.fillWidth: true }
                FutariButton { text: "取消"; enabled: !appController.playlistCreationBusy; onClicked: createPlaylistDialog.reject() }
                FutariButton { text: appController.playlistCreationBusy ? "正在创建…" : "创建并添加"; prominent: true; enabled: newPlaylistName.text.trim().length > 0 && !appController.playlistCreationBusy; onClicked: appController.createPlaylistWithSong(newPlaylistName.text, root.song.id) }
            }
        }
    }
    Connections { target: appController; function onPlaylistCreationFinished(songId, added) { if (String(songId) === String(root.song.id) && createPlaylistDialog.visible) createPlaylistDialog.close() } }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: eventPoint => { const p = root.mapToItem(Overlay.overlay, eventPoint.position.x, eventPoint.position.y); moreMenu.popup(p.x, p.y) } }
    TapHandler { onDoubleTapped: if (!root.inRoom || appController.canControl) root.playRequested() }
}
