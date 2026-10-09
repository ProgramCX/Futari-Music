import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

RowLayout {
    id: root
    property var selectedSongs: []
    property var selectedIds: []
    property int playlistId: 0
    property bool serverPlaylist: false
    property bool canDelete: false
    property bool deleting: false
    signal selectionClearRequested()
    spacing: 10

    function openAddMenu() {
        const point = addButton.mapToItem(Overlay.overlay, 0, addButton.height + 4)
        addMenu.popup(point.x, point.y)
    }

    FutariButton {
        iconName: "play"
        text: "播放"
        enabled: root.selectedIds.length > 0
        onClicked: appController.playSongs(root.selectedIds)
    }
    FutariButton {
        id: addButton
        iconName: "plus"
        text: "添加"
        enabled: root.selectedIds.length > 0
        onClicked: root.openAddMenu()
    }
    FutariButton {
        iconName: "download"
        text: "下载"
        enabled: root.selectedSongs.length > 0
        onClicked: {
            for (const song of root.selectedSongs) appController.enqueueDownload(song)
        }
    }
    FutariButton {
        iconName: "trash"
        text: root.serverPlaylist ? "从歌单移除" : (root.playlistId > 0 ? "从歌单移除" : "删除")
        visible: root.canDelete
        enabled: root.selectedIds.length > 0 && !root.deleting
        onClicked: deleteDialog.open()
    }
    Text {
        text: "已选中 " + root.selectedIds.length + " 首"
        color: Theme.muted
        font.pixelSize: 13
        Layout.leftMargin: 4
    }
    Item { Layout.fillWidth: true }

    FutariMenu {
        id: addMenu
        parent: Overlay.overlay
        FutariMenuItem {
            text: appController.roomState.room && appController.canControl
                  ? "房间共享队列（默认）" : "本地播放队列（默认）"
            iconName: "queue"
            onTriggered: appController.addSongsToQueue(root.selectedIds)
        }
        FutariMenuItem {
            visible: appController.roomState.room && appController.canControl
            text: "本地播放队列"
            iconName: "queue"
            onTriggered: appController.addSongsToLocalQueue(root.selectedIds)
        }
        FutariMenuItem {
            visible: !appController.roomState.room || !appController.canControl
            text: "房间共享队列"
            iconName: "room"
            enabled: !!appController.roomState.room && appController.canControl
            onTriggered: appController.addSongsToQueue(root.selectedIds)
        }
        FutariMenuSeparator {}
        FutariMenuItem {
            text: "创建新歌单并添加"
            iconName: "plus"
            enabled: !appController.playlistCreationBusy
            onTriggered: createPlaylistDialog.open()
        }
        FutariMenuSeparator {}
        Repeater {
            model: appController.playlists.filter(playlist => Number(playlist.id) !== root.playlistId)
            delegate: FutariMenuItem {
                text: modelData.name
                iconName: "playlist"
                onTriggered: appController.addSongsToPlaylist(modelData.id, root.selectedIds, false)
            }
        }
        FutariMenuItem {
            visible: appController.playlists.filter(playlist => Number(playlist.id) !== root.playlistId).length === 0
            text: "还没有其他个人歌单"
            enabled: false
        }
        FutariMenuSeparator {
            visible: appController.canManageServerPlaylist && appController.serverPlaylists.some(
                         playlist => Number(playlist.id) !== root.playlistId)
        }
        Repeater {
            model: appController.canManageServerPlaylist
                   ? appController.serverPlaylists.filter(playlist => Number(playlist.id) !== root.playlistId)
                   : []
            delegate: FutariMenuItem {
                text: "公共 · " + modelData.name
                iconName: "playlist"
                onTriggered: appController.addSongsToPlaylist(modelData.id, root.selectedIds, true)
            }
        }
    }

    FutariDialog {
        id: createPlaylistDialog
        title: "创建新歌单"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 32)
        ColumnLayout {
            width: parent.width
            spacing: 16
            Text {
                text: "创建后会将所选歌曲加入歌单。"
                color: Theme.muted
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            FutariTextField {
                id: playlistName
                placeholderText: "歌单名称"
                maximumLength: 100
                Layout.fillWidth: true
                enabled: !appController.playlistCreationBusy
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                FutariButton {
                    text: "取消"
                    enabled: !appController.playlistCreationBusy
                    onClicked: createPlaylistDialog.reject()
                }
                FutariButton {
                    text: appController.playlistCreationBusy ? "正在创建…" : "创建并添加"
                    prominent: true
                    enabled: playlistName.text.trim().length > 0 && !appController.playlistCreationBusy
                    onClicked: appController.createPlaylistWithSongs(playlistName.text, root.selectedIds)
                }
            }
        }
        Connections {
            target: appController
            function onPlaylistCreationFinished(songId, added) {
                if (root.selectedIds.some(id => String(id) === String(songId)) && createPlaylistDialog.visible)
                    createPlaylistDialog.close()
            }
        }
    }

    FutariDialog {
        id: deleteDialog
        title: root.playlistId > 0 ? "从歌单移除歌曲？" : "删除所选歌曲？"
        parent: Overlay.overlay
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label {
            text: root.playlistId > 0
                  ? "歌曲会从当前歌单移除，曲库中的歌曲保留。"
                  : "这些歌曲将从曲库删除，且无法恢复。"
            color: Theme.text
            wrapMode: Text.Wrap
            padding: 16
        }
        onAccepted: {
            if (root.playlistId > 0)
                appController.removeSongsFromPlaylist(root.playlistId, root.selectedIds, root.serverPlaylist)
            else
                appController.deleteSongs(root.selectedIds)
            root.selectionClearRequested()
        }
    }
}
