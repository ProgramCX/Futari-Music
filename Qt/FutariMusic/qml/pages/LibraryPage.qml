import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    signal editSongRequested(var song)
    signal transfersRequested()
    SingleUploadDialog { id: singleUpload }
    BatchUploadDialog { id: batchUpload }
    Connections { target: singleUpload; function onUploadQueued() { root.transfersRequested() } }
    Connections { target: batchUpload; function onUploadsQueued() { root.transfersRequested() } }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
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
            FutariTextField { id: keyword; placeholderText: "搜索歌曲、歌手或专辑"; Layout.preferredWidth: 320; onAccepted: appController.searchSongs(text) }
            FutariButton { text: "搜索"; onClicked: appController.searchSongs(keyword.text) }
            FutariButton { text: "刷新"; onClicked: appController.searchSongs("") }
            Item { Layout.fillWidth: true }
            Text { text: "上传任务在“上传与下载”中持续运行"; color: Theme.muted; font.pixelSize: 12 }
        }
        Text { text: "曲库 · " + appController.songs.length + " 首"; color: Theme.text; font.pixelSize: 18; font.bold: true }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 3
            model: appController.songs
            delegate: SongRow {
                width: ListView.view.width; song: modelData
                selected: appController.player.song.id === modelData.id
                onPlayRequested: appController.playSong(modelData)
                onQueueRequested: appController.addToQueue(modelData.id)
                onEditRequested: song => root.editSongRequested(song)
            }
            ScrollBar.vertical: ScrollBar {}
        }
        FutariButton { text: "加载更多歌曲"; visible: appController.hasMoreSongs; Layout.alignment: Qt.AlignHCenter; onClicked: appController.loadMoreSongs() }
    }
}
