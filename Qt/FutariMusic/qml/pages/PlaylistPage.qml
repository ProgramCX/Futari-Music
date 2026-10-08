import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    clip: true
    property bool serverMode: false
    property int selectedId: 0
    property var selectedSongIds: []
    property var selectedPlaylist: {
        const entries = serverMode ? appController.serverPlaylists : appController.playlists
        for (let entry of entries) if (entry.id === selectedId) return entry
        return ({})
    }

    Component.onCompleted: refreshCurrentList()
    onServerModeChanged: {
        selectedId = 0
        refreshCurrentList()
    }
    onSelectedPlaylistChanged: {
        if (playlistNameField) playlistNameField.text = root.selectedPlaylist.name || ""
    }

    function refreshCurrentList() {
        if (serverMode) appController.refreshServerPlaylists()
        else appController.refreshPlaylists()
    }

    function createNewPlaylist() {
        const name = newPlaylistName.text.trim()
        if (!name.length) return
        if (serverMode) appController.createServerPlaylist(name)
        else appController.createPlaylist(name)
        newPlaylistName.clear()
    }

    function toggleSelectedSong(songId, selected) {
        const ids = root.selectedSongIds.slice()
        const index = ids.indexOf(songId)
        if (selected && index < 0) ids.push(songId)
        else if (!selected && index >= 0) ids.splice(index, 1)
        root.selectedSongIds = ids
    }

    function addSelectedSongs() {
        if (!root.selectedSongIds.length) return
        appController.addSongsToPlaylist(root.selectedId, root.selectedSongIds, root.serverMode)
        root.selectedSongIds = []
        addSongsDialog.close()
    }

    Row {
        id: playlistColumns
        anchors.fill: parent
        anchors.margins: 28
        spacing: 18

        ColumnLayout {
            id: navigationColumn
            width: Math.min(270, Math.max(190, (playlistColumns.width - playlistColumns.spacing) * 0.28))
            height: playlistColumns.height
            spacing: 14

            Text {
                text: root.serverMode ? "服务器歌单" : "我的歌单"
                color: Theme.text
                font.pixelSize: 28
                font.bold: true
            }
            Text {
                visible: root.serverMode && !appController.canManageServerPlaylist
                text: "浏览所有人可听的公共歌单。"
                color: Theme.muted
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                visible: !root.serverMode || appController.canManageServerPlaylist
                Layout.fillWidth: true
                FutariTextField {
                    id: newPlaylistName
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    placeholderText: root.serverMode ? "新服务器歌单" : "新建个人歌单"
                    onAccepted: root.createNewPlaylist()
                }
                FutariToolButton {
                    id: createPlaylistButton
                    iconName: "plus"
                    ToolTip.visible: hovered
                    ToolTip.text: "创建歌单"
                    onClicked: root.createNewPlaylist()
                }
            }

            ListView {
                id: playlistList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 5
                model: root.serverMode ? appController.serverPlaylists : appController.playlists
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 48
                    radius: Theme.radiusSmall
                    color: modelData.id === root.selectedId ? Theme.secondary
                           : (playlistHover.hovered ? Theme.hover : "transparent")
                    HoverHandler { id: playlistHover }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8
                        SvgIcon {
                            name: "playlist"
                            iconSize: 17
                            color: modelData.id === root.selectedId ? Theme.iconAccent : Theme.iconSecondary
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: Theme.text
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                        Text {
                            text: String((modelData.songs || []).length)
                            color: Theme.muted
                            font.pixelSize: 12
                        }
                    }
                    TapHandler { onTapped: root.selectedId = modelData.id }
                }
                ScrollBar.vertical: ScrollBar {}
            }
        }

        Rectangle {
            width: Math.max(0, playlistColumns.width - navigationColumn.width - playlistColumns.spacing)
            height: playlistColumns.height
            radius: Theme.radius
            color: Theme.surface
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: root.selectedPlaylist.name || (root.serverMode ? "选择服务器歌单" : "选择个人歌单")
                        color: Theme.text
                        font.pixelSize: 21
                        font.bold: true
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    FutariButton {
                        text: "刷新"
                        onClicked: root.refreshCurrentList()
                    }
                    FutariButton {
                        text: "添加歌曲"
                        iconName: "plus"
                        visible: !!root.selectedPlaylist.id && (!root.serverMode || appController.canManageServerPlaylist)
                        onClicked: {
                            root.selectedSongIds = []
                            songKeyword.clear()
                            appController.searchSongs("")
                            addSongsDialog.open()
                        }
                    }
                    FutariButton {
                        text: "删除歌单"
                        visible: !!root.selectedPlaylist.id && (!root.serverMode || appController.canManageServerPlaylist)
                        onClicked: deletePlaylistDialog.open()
                    }
                }

                RowLayout {
                    visible: !!root.selectedPlaylist.id && (!root.serverMode || appController.canManageServerPlaylist)
                    Layout.fillWidth: true
                    FutariTextField {
                        id: playlistNameField
                        Layout.preferredWidth: 280
                        placeholderText: "歌单名称"
                        text: root.selectedPlaylist.name || ""
                    }
                    FutariButton {
                        text: "保存名称"
                        onClicked: appController.updatePlaylist(root.selectedId, playlistNameField.text,
                                                               root.selectedPlaylist.description || "",
                                                               root.selectedPlaylist.coverUrl || "", root.serverMode)
                    }
                }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !root.selectedPlaylist.id || !(root.selectedPlaylist.songs || []).length
                        Column {
                            anchors.centerIn: parent
                            width: Math.min(420, Math.max(0, parent.width - 40))
                            spacing: 8
                            Text {
                                width: parent.width
                                text: root.selectedPlaylist.id ? "歌单还是空的" : "先从左侧选择一个歌单"
                                color: Theme.text
                                font.pixelSize: 16
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                            }
                            Text {
                                width: parent.width
                                text: root.selectedPlaylist.id ? "添加歌曲后会出现在这里。" : ""
                                color: Theme.muted
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                            }
                        }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: !!root.selectedPlaylist.id && (root.selectedPlaylist.songs || []).length > 0
                    clip: true
                    spacing: 3
                    model: root.selectedPlaylist.songs || []
                    delegate: RowLayout {
                        width: ListView.view.width
                        spacing: 6
                        SongRow {
                            Layout.fillWidth: true
                            song: modelData
                            onPlayRequested: appController.playSong(modelData)
                            onQueueRequested: appController.addToQueue(modelData.id)
                        }
                        FutariToolButton {
                            iconName: "queue-up"
                            enabled: index > 0 && (!root.serverMode || appController.canManageServerPlaylist)
                            onClicked: root.moveSong(index, -1)
                            ToolTip.visible: hovered
                            ToolTip.text: "上移"
                        }
                        FutariToolButton {
                            iconName: "queue-down"
                            enabled: index < (root.selectedPlaylist.songs || []).length - 1
                                     && (!root.serverMode || appController.canManageServerPlaylist)
                            onClicked: root.moveSong(index, 1)
                            ToolTip.visible: hovered
                            ToolTip.text: "下移"
                        }
                        FutariToolButton {
                            iconName: "remove"
                            visible: !root.serverMode || appController.canManageServerPlaylist
                            onClicked: appController.removeSongFromPlaylist(root.selectedPlaylist.id, modelData.id, root.serverMode)
                            ToolTip.visible: hovered
                            ToolTip.text: "从歌单移除"
                        }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }
    }

    FutariDialog {
        id: addSongsDialog
        title: "从曲库添加歌曲"
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 700
        height: 590
        footer: RowLayout {
            spacing: 10
            Item { Layout.fillWidth: true }
            FutariButton { text: "关闭"; onClicked: addSongsDialog.close() }
            FutariButton {
                text: "添加已选（" + root.selectedSongIds.length + "）"
                prominent: true
                enabled: root.selectedSongIds.length > 0
                onClicked: root.addSelectedSongs()
            }
        }
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                FutariTextField {
                    id: songKeyword
                    Layout.fillWidth: true
                    placeholderText: "搜索歌曲、歌手或专辑"
                    onAccepted: appController.searchSongs(text)
                }
                FutariButton { text: "搜索"; onClicked: appController.searchSongs(songKeyword.text) }
            }
            Text {
                text: "曲库 · " + appController.songs.length + " 首"
                color: Theme.muted
                font.pixelSize: 13
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 2
                model: appController.songs
                delegate: RowLayout {
                    width: ListView.view.width
                    FutariCheckBox {
                        text: ""
                        checked: root.selectedSongIds.indexOf(modelData.id) >= 0
                        onClicked: root.toggleSelectedSong(modelData.id, checked)
                    }
                    SongRow {
                        Layout.fillWidth: true
                        song: modelData
                        onPlayRequested: appController.playSong(modelData)
                        onQueueRequested: appController.addToQueue(modelData.id)
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }
            FutariButton {
                text: "加载更多歌曲"
                visible: appController.hasMoreSongs
                Layout.alignment: Qt.AlignHCenter
                onClicked: appController.loadMoreSongs()
            }
        }
    }

    FutariDialog {
        id: deletePlaylistDialog
        title: "删除歌单？"
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }
        Label {
            text: "删除后，歌单中的歌曲不会从曲库删除。"
            color: Theme.text
            padding: 18
        }
        onAccepted: {
            appController.deletePlaylist(root.selectedId, root.serverMode)
            root.selectedId = 0
        }
    }

    function moveSong(index, direction) {
        const ids = (root.selectedPlaylist.songs || []).map(song => song.id)
        const target = index + direction
        if (target < 0 || target >= ids.length) return
        const value = ids[index]
        ids[index] = ids[target]
        ids[target] = value
        appController.reorderPlaylist(root.selectedPlaylist.id, ids, root.serverMode)
    }
}
