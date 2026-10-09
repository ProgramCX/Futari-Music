import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    signal editSongRequested(var song)
    signal transfersRequested()
    property bool batchMode: false
    property var selectedSongIds: []
    readonly property var allSongs: appController.songs
    SingleUploadDialog { id: singleUpload }
    BatchUploadDialog { id: batchUpload }
    Connections { target: singleUpload; function onUploadQueued() { root.transfersRequested() } }
    Connections { target: batchUpload; function onUploadsQueued() { root.transfersRequested() } }

    function clearBatchSelection() { selectedSongIds = [] }
    function toggleSongSelection(songId, checked) {
        const ids = selectedSongIds.slice()
        const index = ids.indexOf(Number(songId))
        if (checked && index < 0) ids.push(Number(songId))
        else if (!checked && index >= 0) ids.splice(index, 1)
        selectedSongIds = ids
    }
    function selectedSongs() {
        return allSongs.filter(song => selectedSongIds.indexOf(Number(song.id)) >= 0)
    }
    function setAllSelected(checked) {
        const visibleIds = allSongs.map(song => Number(song.id))
        selectedSongIds = checked ? visibleIds : []
    }
    function runSearch(value) {
        clearBatchSelection()
        appController.searchSongs(value)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 14
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Text { text: "发现音乐"; color: Theme.text; font.pixelSize: 32; font.bold: true }
                Text { text: "一起发现喜欢的声音"; color: Theme.muted }
            }
            Item { Layout.fillWidth: true }
            UploadMenuButton {
                allowed: appController.canUpload
                onSingleRequested: singleUpload.open()
                onBatchRequested: batchUpload.open()
            }
        }
        RowLayout {
            FutariTextField {
                id: keyword
                placeholderText: "搜索歌曲、歌手或专辑"
                Layout.preferredWidth: 320
                onAccepted: root.runSearch(text)
            }
            FutariButton { text: "搜索"; onClicked: root.runSearch(keyword.text) }
            FutariButton { text: "刷新"; onClicked: root.runSearch("") }
            Item { Layout.fillWidth: true }
            Text { text: "上传任务在“上传与下载”中持续运行"; color: Theme.muted; font.pixelSize: 12 }
        }
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "曲库 · " + appController.songs.length + " 首"
                color: Theme.text
                font.pixelSize: 18
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            FutariButton {
                text: root.batchMode ? "退出批量操作" : "批量操作"
                iconName: root.batchMode ? "window-close" : "queue"
                onClicked: {
                    root.batchMode = !root.batchMode
                    if (!root.batchMode) root.clearBatchSelection()
                }
            }
        }
        RowLayout {
            visible: root.batchMode
            Layout.fillWidth: true
            FutariCheckBox {
                text: "全选当前列表"
                checked: root.allSongs.length > 0 && root.allSongs.every(song => root.selectedSongIds.indexOf(Number(song.id)) >= 0)
                onClicked: root.setAllSelected(checked)
            }
            BatchSongToolbar {
                Layout.fillWidth: true
                selectedIds: root.selectedSongIds
                selectedSongs: root.selectedSongs()
                canDelete: appController.admin
                deleting: appController.batchDeletionBusy
                onSelectionClearRequested: root.clearBatchSelection()
            }
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 3
            model: appController.songs
            delegate: RowLayout {
                width: ListView.view.width
                spacing: 4
                FutariCheckBox {
                    visible: root.batchMode
                    text: ""
                    checked: root.selectedSongIds.indexOf(Number(modelData.id)) >= 0
                    onClicked: root.toggleSongSelection(modelData.id, checked)
                }
                SongRow {
                    Layout.fillWidth: true
                    song: modelData
                    selected: appController.player.song.id === modelData.id
                    onPlayRequested: appController.playSong(modelData)
                    onQueueRequested: appController.addToQueue(modelData.id)
                    onEditRequested: song => root.editSongRequested(song)
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
