import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

Item {
    id: root
    property var song: ({})
    property string audioPath: ""
    property string coverPath: ""
    property string lyricsText: ""
    property string karaokeLyricsText: ""
    property string lineLyricsName: ""
    property string karaokeLyricsName: ""
    property string karaokeLyricsFileError: ""
    // Dirty flags distinguish an untouched value from an intentional clear in the multipart update.
    property bool lineLyricsChanged: false
    property bool karaokeLyricsChanged: false
    property bool clearLineLyrics: false
    property bool clearKaraokeLyrics: false
    property bool loadingLyrics: false
    property string albumName: ""
    property string newAlbumName: ""
    property bool newAlbumMode: false
    property int selectedAlbumId: 0
    property var albumOptions: [{ "id": 0, "name": "不关联专辑" }].concat(appController.albums)
    signal backRequested()

    function selectAlbum() {
        var selected = 0
        for (var i = 0; i < albumOptions.length; ++i) {
            if (Number(albumOptions[i].id) === Number(selectedAlbumId)) { selected = i; break }
        }
        albumSelector.currentIndex = selected
    }
    function loadSong() {
        audioPath = ""; coverPath = ""; newAlbumName = ""; newAlbumMode = false; newAlbumField.text = ""
        titleField.text = song.title || ""
        artistField.text = song.artist || ""
        selectedAlbumId = Number(song.albumId || 0)
        albumName = song.album || ""
        root.loadingLyrics = true
        lyricsText = ""; karaokeLyricsText = ""
        lineLyricsName = ""; karaokeLyricsName = ""
        karaokeLyricsFileError = ""
        lineLyricsChanged = false; karaokeLyricsChanged = false
        clearLineLyrics = false; clearKaraokeLyrics = false
        lyricsArea.text = ""; karaokeLyricsArea.text = ""
        root.loadingLyrics = false
        selectAlbum()
        if (song.id) appController.loadLyrics(song.id)
    }

    onSongChanged: if (visible) loadSong()
    onVisibleChanged: if (visible && song.id) loadSong()
    Connections {
        target: appController
        function onLyricsChanged() {
            if (Number(appController.lyrics.songId) === Number(root.song.id)) {
                root.loadingLyrics = true
                root.lyricsText = appController.lyrics.lyrics || ""
                root.karaokeLyricsText = appController.lyrics.karaokeLyrics || ""
                lyricsArea.text = root.lyricsText
                karaokeLyricsArea.text = root.karaokeLyricsText
                root.lineLyricsChanged = false; root.karaokeLyricsChanged = false
                root.clearLineLyrics = false; root.clearKaraokeLyrics = false
                root.loadingLyrics = false
            }
        }
        function onAlbumsChanged() { root.selectAlbum() }
    }

    FileDialog {
        id: audioPicker; title: "替换歌曲音频"; nameFilters: ["音频 (*.mp3 *.flac *.aac *.ogg *.wav *.m4a)"]
        onAccepted: root.audioPath = selectedFile.toString()
    }
    FileDialog {
        id: lyricsPicker; title: "选择歌词文件（可多选，按内容识别）"; fileMode: FileDialog.OpenFiles
        nameFilters: ["歌词文件 (*.lrc *.txt)"]
        onAccepted: {
            for (const selected of selectedFiles) {
                const url = selected.toString()
                const text = appController.readLyricsFile(url)
                const name = decodeURIComponent(url.split("/").pop())
                if (appController.classifyLyrics(text) === "karaoke") {
                    root.karaokeLyricsName = name; karaokeLyricsArea.text = text
                    root.karaokeLyricsChanged = true; root.clearKaraokeLyrics = false
                } else {
                    root.lineLyricsName = name; lyricsArea.text = text
                    root.lineLyricsChanged = true; root.clearLineLyrics = false
                }
            }
        }
    }
    FileDialog {
        id: karaokeLyricsPicker
        title: "选择逐字歌词文件"
        nameFilters: ["逐字歌词 (*.lrc *.txt)"]
        onAccepted: {
            const url = selectedFile.toString()
            const text = appController.readLyricsFile(url)
            if (appController.classifyLyrics(text) !== "karaoke") {
                root.karaokeLyricsFileError = "文件中没有识别到逐字时间标签，请选择逐字歌词文件。"
                return
            }
            root.karaokeLyricsName = decodeURIComponent(url.split("/").pop())
            root.karaokeLyricsFileError = ""
            karaokeLyricsArea.text = text
            root.karaokeLyricsChanged = true
            root.clearKaraokeLyrics = false
        }
    }
    FileDialog {
        id: coverPicker; title: "更换专辑图片"; nameFilters: ["图片 (*.jpg *.jpeg *.png *.webp)"]
        onAccepted: root.coverPath = selectedFile.toString()
    }
    FutariDialog {
        id: deleteDialog; title: "删除歌曲"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: "确定从曲库删除《" + (root.song.title || "未命名歌曲") + "》吗？"; color: Theme.text; wrapMode: Text.Wrap; width: 330 }
        onAccepted: appController.deleteSong(root.song.id)
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 30; spacing: 18
        RowLayout {
            Layout.fillWidth: true
            FutariToolButton { iconName: "previous"; iconColor: Theme.iconPrimary; onClicked: root.backRequested(); ToolTip.visible: hovered; ToolTip.text: "返回曲库" }
            ColumnLayout {
                spacing: 3
                Text { text: "编辑歌曲"; color: Theme.text; font.pixelSize: 29; font.bold: true }
                Text { text: "修改曲库歌曲与其专辑信息"; color: Theme.muted; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            FutariButton {
                text: "删除歌曲"; iconName: "trash"; prominent: true; accent: Theme.danger
                visible: appController.admin; onClicked: deleteDialog.open()
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; radius: Theme.radiusLarge
            color: Theme.surface; border.color: Theme.border; border.width: 1
            Flickable {
                anchors.fill: parent; anchors.margins: 24; clip: true
                contentWidth: width; contentHeight: form.implicitHeight
                ScrollBar.vertical: ScrollBar {}
                ColumnLayout {
                    id: form; width: parent.width; spacing: 12
                    RowLayout {
                        Layout.fillWidth: true; spacing: 16
                        Rectangle {
                            Layout.preferredWidth: 92; Layout.preferredHeight: 92
                            radius: Theme.radius; color: Theme.secondary; clip: true
                            SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 34; visible: coverPreview.status !== Image.Ready }
                            Image { id: coverPreview; anchors.fill: parent; source: appController.covers[String(root.song.id)] || ""; fillMode: Image.PreserveAspectCrop }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 5
                            Text { text: root.song.artist || "未知歌手"; color: Theme.muted; font.pixelSize: 14 }
                            Text { text: root.song.format ? root.song.format.toUpperCase() + " · " + Math.round(Number(root.song.fileSize || 0) / 1024) + " KB" : "歌曲文件"; color: Theme.muted; font.pixelSize: 12 }
                            FutariButton { text: root.audioPath ? "已选择替换音频" : "替换音频文件"; iconName: "upload"; onClicked: audioPicker.open() }
                        }
                    }
                    Text { text: "歌曲名称"; color: Theme.text; font.weight: Font.Medium }
                    FutariTextField { id: titleField; placeholderText: "歌曲名称"; Layout.fillWidth: true }
                    Text { text: "歌手"; color: Theme.text; font.weight: Font.Medium }
                    FutariTextField { id: artistField; placeholderText: "歌手"; Layout.fillWidth: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "专辑"; color: Theme.text; font.weight: Font.Medium }
                        Item { Layout.fillWidth: true }
                        FutariButton {
                            text: root.newAlbumMode ? "选择已有专辑" : "新建专辑"
                            onClicked: {
                                root.newAlbumMode = !root.newAlbumMode
                                if (root.newAlbumMode) { root.selectedAlbumId = 0; root.albumName = "" }
                                else { root.newAlbumName = ""; newAlbumField.text = ""; root.selectAlbum() }
                            }
                        }
                    }
                    FutariComboBox {
                        id: albumSelector; visible: !root.newAlbumMode; Layout.fillWidth: true
                        model: root.albumOptions; textRole: "name"; currentIndex: 0
                        onActivated: index => {
                            root.selectedAlbumId = Number(root.albumOptions[index].id)
                            root.albumName = root.albumOptions[index].name
                        }
                    }
                    FutariTextField {
                        id: newAlbumField; visible: root.newAlbumMode; Layout.fillWidth: true
                        placeholderText: "新专辑名称"; text: root.newAlbumName
                        onTextEdited: root.newAlbumName = text
                    }
                    FutariTextField {
                        visible: !root.newAlbumMode && root.selectedAlbumId > 0
                        Layout.fillWidth: true; placeholderText: "专辑名称"; text: root.albumName
                        onTextEdited: root.albumName = text
                    }
                    Text {
                        visible: !root.newAlbumMode && root.selectedAlbumId > 0
                        text: "更改专辑名称或图片会同步影响此专辑下的所有歌曲。"
                        color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true
                    }
                    FutariButton {
                        text: root.coverPath ? "已选择新的专辑图片" : "更换专辑图片（可选）"
                        iconName: "upload"; Layout.alignment: Qt.AlignLeft; onClicked: coverPicker.open()
                    }
                    Text { text: "逐行歌词"; color: Theme.text; font.weight: Font.Medium }
                    TextArea {
                        id: lyricsArea; Layout.fillWidth: true; Layout.preferredHeight: 180
                        placeholderText: "编辑逐行歌词，支持 LRC 时间标签"
                        color: Theme.text; placeholderTextColor: Theme.muted
                        wrapMode: TextEdit.Wrap; selectByMouse: true
                        background: Rectangle { radius: Theme.radiusSmall; color: Theme.fieldFill; border.color: lyricsArea.activeFocus ? Theme.fieldFocusBorder : Theme.fieldBorder }
                        onTextChanged: {
                            root.lyricsText = text
                            if (!root.loadingLyrics) {
                                root.lineLyricsChanged = true
                                root.clearLineLyrics = text.trim().length === 0
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FutariButton { text: "选择歌词文件（自动识别）"; onClicked: lyricsPicker.open() }
                        Text { text: root.lineLyricsName || "支持直接粘贴或编辑"; color: Theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        FutariButton {
                            text: "清除逐行歌词"; enabled: lyricsArea.text.length > 0
                            onClicked: { root.lineLyricsName = ""; lyricsArea.text = ""; root.lineLyricsChanged = true; root.clearLineLyrics = true }
                        }
                    }
                    Text { text: "逐字歌词"; color: Theme.text; font.weight: Font.Medium }
                    TextArea {
                        id: karaokeLyricsArea; Layout.fillWidth: true; Layout.preferredHeight: 180
                        placeholderText: "编辑逐字 LRC，字符时间标签格式如 <00:01.23>"
                        color: Theme.text; placeholderTextColor: Theme.muted
                        wrapMode: TextEdit.Wrap; selectByMouse: true
                        background: Rectangle { radius: Theme.radiusSmall; color: Theme.fieldFill; border.color: karaokeLyricsArea.activeFocus ? Theme.fieldFocusBorder : Theme.fieldBorder }
                        onTextChanged: {
                            root.karaokeLyricsText = text
                            if (!root.loadingLyrics) {
                                root.karaokeLyricsChanged = true
                                root.clearKaraokeLyrics = text.trim().length === 0
                            }
                        }
                    }
                    Text {
                        visible: appController.countLikelyCreditLines(root.lyricsText + "\n" + root.karaokeLyricsText) > 0
                        text: "可能包含演职员信息行；解析不会自动删掉这些内容，可在此编辑确认。"
                        color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true
                    }
                    Text {
                        visible: root.karaokeLyricsFileError.length > 0
                        text: root.karaokeLyricsFileError
                        color: Theme.danger
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: root.karaokeLyricsName || "有逐字歌词时自动显示卡拉 OK 效果"; color: Theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        FutariButton { text: "选择逐字歌词文件"; iconName: "upload"; onClicked: karaokeLyricsPicker.open() }
                        FutariButton {
                            text: "清除逐字歌词"; enabled: karaokeLyricsArea.text.length > 0
                            onClicked: { root.karaokeLyricsName = ""; root.karaokeLyricsFileError = ""; karaokeLyricsArea.text = ""; root.karaokeLyricsChanged = true; root.clearKaraokeLyrics = true }
                        }
                        Item { Layout.fillWidth: true }
                        FutariButton { text: "取消"; onClicked: root.backRequested() }
                        FutariButton {
                            text: "保存更改"; prominent: true; enabled: titleField.text.trim().length > 0
                            onClicked: {
                                const lyricChanges = ({})
                                if (root.lineLyricsChanged) {
                                    if (root.clearLineLyrics) lyricChanges.clearLyrics = true
                                    else lyricChanges.lyricsText = root.lyricsText
                                }
                                if (root.karaokeLyricsChanged) {
                                    if (root.clearKaraokeLyrics) lyricChanges.clearKaraokeLyrics = true
                                    else lyricChanges.karaokeLyricsText = root.karaokeLyricsText
                                }
                                appController.updateSongWithLyrics(root.song.id, root.audioPath, titleField.text, artistField.text,
                                    root.selectedAlbumId, root.albumName, root.newAlbumMode ? root.newAlbumName : "",
                                    lyricChanges, root.coverPath)
                            }
                        }
                    }
                }
            }
        }
    }
}
