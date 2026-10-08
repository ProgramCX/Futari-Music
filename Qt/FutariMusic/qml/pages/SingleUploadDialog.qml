import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

FutariDialog {
    id: root
    title: "单曲上传"
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent.width - 28, 650)
    height: Math.min(parent.height - 28, 720)
    padding: 22
    property string audioUrl: ""
    property string lyricsUrl: ""
    property string lyricsFileName: ""
    property string manualCoverUrl: ""
    property string embeddedCoverUrl: ""
    property string metadataAlbum: ""
    property string metadataAlbumArtist: ""
    property string pendingRequestId: ""
    property var trackCandidates: []
    property var albumChoices: [{ id: 0, name: "不关联专辑", artist: "", displayName: "不关联专辑" }]
    property int selectedAlbumId: 0
    property int durationMs: 0
    property bool titleEdited: false
    property bool artistEdited: false
    property bool albumEdited: false
    property bool newAlbumMode: false
    property bool useEmbeddedCover: false
    signal uploadQueued()

    function resetForm() {
        audioUrl = ""; lyricsUrl = ""; lyricsFileName = ""; manualCoverUrl = ""; embeddedCoverUrl = ""
        metadataAlbum = ""; metadataAlbumArtist = ""; pendingRequestId = ""; trackCandidates = []
        selectedAlbumId = 0; durationMs = 0; titleEdited = false; artistEdited = false; albumEdited = false
        newAlbumMode = false; useEmbeddedCover = false
        titleField.text = ""; artistField.text = ""
        lyricsArea.text = ""; newAlbumNameField.text = ""; newAlbumArtistField.text = ""
        metadataStatus.text = "选择音频后读取内嵌标签和封面"
        lyricsStatus.text = "未找到歌词"
        autoMetadata.checked = true; autoLyrics.checked = true
        albumChoices = [{ id: 0, name: "不关联专辑", artist: "", displayName: "不关联专辑" }]
    }
    function setAlbumChoices(rows) {
        var choices = [{ id: 0, name: "不关联专辑", artist: "", displayName: "不关联专辑" }]
        for (var i = 0; i < rows.length; ++i) {
            var item = rows[i]
            choices.push({ id: Number(item.id), name: item.name || "", artist: item.artist || "",
                           displayName: (item.name || "未命名专辑") + (item.artist ? " · " + item.artist : "") })
        }
        root.albumChoices = choices
    }
    function selectedCover() {
        if (manualCoverUrl.length > 0) return manualCoverUrl
        if (newAlbumMode || selectedAlbumId === 0 || useEmbeddedCover) return embeddedCoverUrl
        return ""
    }
    function selectedSongCover() {
        if (selectedAlbumId === 0 && !newAlbumMode) return manualCoverUrl || embeddedCoverUrl
        return embeddedCoverUrl
    }
    function startMetadataRead() {
        if (root.audioUrl.length === 0 || !autoMetadata.checked) return
        root.pendingRequestId = Date.now().toString() + Math.random().toString().slice(2)
        appController.inspectAudio(root.audioUrl, root.pendingRequestId)
    }
    function searchLyrics() {
        if (root.audioUrl.length === 0 || !autoLyrics.checked) return
        const match = appController.findLyricsForAudio(root.audioUrl)
        if (match.found) {
            root.lyricsUrl = match.url; root.lyricsFileName = match.name
            lyricsArea.text = appController.readLyricsFile(match.url)
            lyricsStatus.text = "已匹配歌词：" + match.name
        } else {
            root.lyricsUrl = ""; root.lyricsFileName = ""
            lyricsStatus.text = "未找到同名歌词；仍可直接填写或手动选择文件"
        }
    }
    function applyAlbumMatches() {
        const matches = appController.albumMatches
        setAlbumChoices(matches)
        let bestIndex = -1
        let artistMatches = []
        const matchArtist = metadataAlbumArtist || artistField.text.trim()
        for (let i = 0; i < matches.length; ++i) {
            if (matchArtist.length > 0 && String(matches[i].artist || "").toLocaleLowerCase() === matchArtist.toLocaleLowerCase())
                artistMatches.push(i)
        }
        if (artistMatches.length === 1) bestIndex = artistMatches[0] + 1
        else if (matchArtist.length === 0 && matches.length === 1) bestIndex = 1
        else bestIndex = 0
        albumSelector.currentIndex = bestIndex
        selectedAlbumId = bestIndex > 0 ? Number(albumChoices[bestIndex].id) : 0
    }

    FileDialog {
        id: audioPicker; title: "选择音频文件"; nameFilters: ["音频 (*.mp3 *.flac *.aac *.ogg *.wav *.m4a)"]
        onAccepted: {
            root.audioUrl = selectedFile.toString()
            root.trackCandidates = appController.guessTrackMetadata(root.audioUrl)
            if (root.trackCandidates.length > 0) {
                if (!root.titleEdited || titleField.text.length === 0) titleField.text = root.trackCandidates[0].title
                if (!root.artistEdited || artistField.text.length === 0) artistField.text = root.trackCandidates[0].artist
            }
            root.searchLyrics()
            root.startMetadataRead()
        }
    }
    FileDialog {
        id: lyricsPicker; title: "选择歌词文件"; nameFilters: ["歌词 (*.lrc *.txt)"]
        onAccepted: {
            root.lyricsUrl = selectedFile.toString(); root.lyricsFileName = selectedFile.toString().split("/").pop()
            lyricsArea.text = appController.readLyricsFile(root.lyricsUrl)
            lyricsStatus.text = "已选择歌词：" + root.lyricsFileName
        }
    }
    FileDialog {
        id: coverPicker; title: "选择歌曲或专辑图片"; nameFilters: ["图片 (*.jpg *.jpeg *.png *.webp)"]
        onAccepted: root.manualCoverUrl = selectedFile.toString()
    }
    Connections {
        target: appController
        function onAudioMetadataReady(requestId, metadata) {
            if (requestId !== root.pendingRequestId || !autoMetadata.checked) return
            if (!root.titleEdited && metadata.title) titleField.text = metadata.title
            if (!root.artistEdited && metadata.artist) artistField.text = metadata.artist
            if (!root.albumEdited && metadata.album) {
                root.metadataAlbum = metadata.album
                root.metadataAlbumArtist = metadata.albumArtist || ""
                appController.searchAlbumMatches(metadata.album, root.metadataAlbumArtist || artistField.text.trim())
            }
            if (metadata.coverUrl) root.embeddedCoverUrl = metadata.coverUrl
            root.durationMs = Number(metadata.durationMs || 0)
            metadataStatus.text = metadata.available ? "已读取音频标签" : "未读取到标签，已使用文件名作为歌名"
        }
        function onAlbumMatchesChanged() { if (root.metadataAlbum.length > 0 && !root.albumEdited) root.applyAlbumMatches() }
    }
    Connections {
        target: autoMetadata
        function onToggled() { if (autoMetadata.checked) root.startMetadataRead() }
    }
    Connections {
        target: autoLyrics
        function onToggled() { if (autoLyrics.checked) root.searchLyrics() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Flickable {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: width; contentHeight: fields.implicitHeight
            ScrollBar.vertical: ScrollBar {}
            ColumnLayout {
                id: fields; width: parent.width; spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    FutariCheckBox { id: autoMetadata; text: "根据元信息自动填充"; checked: true }
                    FutariCheckBox { id: autoLyrics; text: "自动查找歌词"; checked: true }
                }
                FutariButton { Layout.fillWidth: true; text: root.audioUrl ? root.audioUrl.split("/").pop() : "选择音频文件 *"; iconName: "upload"; onClicked: audioPicker.open() }
                Text { id: metadataStatus; objectName: "metadataStatus"; text: "选择音频后读取内嵌标签和封面"; color: Theme.muted; font.pixelSize: 12 }
                FutariTextField {
                    id: titleField; placeholderText: "歌曲名称 *"; Layout.fillWidth: true
                    onTextEdited: root.titleEdited = true
                    onActiveFocusChanged: if (activeFocus && root.trackCandidates.length > 1) titleAlternatives.visible = true
                }
                FutariComboBox {
                    id: titleAlternatives; visible: false; Layout.fillWidth: true
                    model: root.trackCandidates; textRole: "label"
                    onActivated: index => { titleField.text = root.trackCandidates[index].title; artistField.text = root.trackCandidates[index].artist; root.titleEdited = true; root.artistEdited = true; visible = false }
                }
                FutariTextField {
                    id: artistField; objectName: "songArtist"; placeholderText: "歌手"; Layout.fillWidth: true
                    onTextEdited: root.artistEdited = true
                    onActiveFocusChanged: if (activeFocus && root.trackCandidates.length > 1) artistAlternatives.visible = true
                }
                FutariComboBox {
                    id: artistAlternatives; visible: false; Layout.fillWidth: true
                    model: root.trackCandidates; textRole: "label"
                    onActivated: index => { titleField.text = root.trackCandidates[index].title; artistField.text = root.trackCandidates[index].artist; root.titleEdited = true; root.artistEdited = true; visible = false }
                }
                RowLayout {
                    Layout.fillWidth: true
                    FutariComboBox {
                        id: albumSelector; Layout.fillWidth: true; model: root.albumChoices; textRole: "displayName"; currentIndex: 0
                        visible: !root.newAlbumMode
                        onActivated: index => { root.selectedAlbumId = index > 0 ? Number(root.albumChoices[index].id) : 0; root.albumEdited = true }
                    }
                    FutariButton {
                        objectName: "toggleNewAlbum"
                        text: root.newAlbumMode ? "选择已有专辑"
                                                 : (root.metadataAlbum.length > 0 ? "创建“" + root.metadataAlbum + "”专辑" : "创建新专辑")
                        onClicked: {
                            root.newAlbumMode = !root.newAlbumMode
                            if (root.newAlbumMode) {
                                root.selectedAlbumId = 0
                                if (!newAlbumNameField.text && root.metadataAlbum) newAlbumNameField.text = root.metadataAlbum
                                if (!newAlbumArtistField.text) newAlbumArtistField.text = root.metadataAlbumArtist || artistField.text.trim()
                                albumEdited = true
                            } else { newAlbumNameField.text = ""; newAlbumArtistField.text = "" }
                        }
                    }
                }
                FutariTextField { id: newAlbumNameField; visible: root.newAlbumMode; Layout.fillWidth: true; placeholderText: "新专辑名称" }
                FutariTextField { id: newAlbumArtistField; objectName: "newAlbumArtist"; visible: root.newAlbumMode; Layout.fillWidth: true; placeholderText: "专辑艺术家（可选）" }
                Text {
                    visible: root.metadataAlbum.length > 0 && !root.newAlbumMode
                    text: root.albumChoices.length > 1
                          ? (root.selectedAlbumId > 0
                             ? "已匹配曲库专辑；也可以从列表更改选择。"
                             : "已识别专辑：" + root.metadataAlbum + "。曲库存在多个候选，请选择匹配项或创建该专辑。")
                          : "已识别专辑：" + root.metadataAlbum + "。曲库没有匹配项；可创建该专辑，也可保持不关联。"
                    color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    Rectangle {
                        width: 72; height: 72; radius: Theme.radiusSmall; color: Theme.secondary; clip: true
                        SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 28; visible: coverPreview.status !== Image.Ready }
                        Image { id: coverPreview; objectName: "embeddedCoverPreview"; anchors.fill: parent; source: root.manualCoverUrl || root.embeddedCoverUrl; fillMode: Image.PreserveAspectCrop }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: root.embeddedCoverUrl ? "已发现音频内嵌封面" : "没有内嵌封面，可选择图片"; color: Theme.muted; font.pixelSize: 12 }
                        FutariButton { text: root.manualCoverUrl ? "已选择自定义封面" : "选择封面"; iconName: "upload"; onClicked: coverPicker.open() }
                        FutariCheckBox {
                            visible: root.selectedAlbumId > 0 && root.embeddedCoverUrl.length > 0 && root.manualCoverUrl.length === 0
                            text: "使用内嵌图更新共享专辑封面"; checked: root.useEmbeddedCover
                            onToggled: root.useEmbeddedCover = checked
                        }
                    }
                }
                Text { text: "歌词"; color: Theme.text; font.weight: Font.Medium }
                Text {
                    visible: !root.newAlbumMode && root.selectedAlbumId > 0 && root.selectedCover().length > 0
                    text: "更换现有专辑封面会同步影响该专辑下的所有歌曲。"
                    color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true
                }
                Text { id: lyricsStatus; text: "未找到歌词"; color: Theme.muted; font.pixelSize: 12 }
                TextArea {
                    id: lyricsArea; Layout.fillWidth: true; Layout.preferredHeight: 115
                    placeholderText: "可直接填写歌词，支持 LRC 时间标签"; wrapMode: TextEdit.Wrap; selectByMouse: true
                    color: Theme.text; placeholderTextColor: Theme.muted
                    background: Rectangle { radius: Theme.radiusSmall; color: Theme.fieldFill; border.color: lyricsArea.activeFocus ? Theme.fieldFocusBorder : Theme.fieldBorder }
                }
                RowLayout {
                    FutariButton { text: root.lyricsFileName ? "替换歌词文件" : "选择歌词文件"; onClicked: lyricsPicker.open() }
                    FutariButton { text: "移除歌词"; enabled: root.lyricsFileName.length > 0 || lyricsArea.text.length > 0; onClicked: { root.lyricsUrl = ""; root.lyricsFileName = ""; lyricsArea.text = ""; lyricsStatus.text = "未找到歌词" } }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            FutariButton { text: "取消"; onClicked: root.reject() }
            FutariButton {
                text: "加入上传队列"; prominent: true
                enabled: root.audioUrl.length > 0 && titleField.text.trim().length > 0
                onClicked: {
                    const options = root.albumChoices
                    const chosen = albumSelector.currentIndex > 0 && !root.newAlbumMode ? options[albumSelector.currentIndex] : null
                    const taskId = appController.enqueueUpload({
                        audioUrl: root.audioUrl, title: titleField.text.trim(), artist: artistField.text.trim(),
                        durationMs: root.durationMs, album: root.metadataAlbum,
                        albumId: chosen ? chosen.id : 0,
                        newAlbumName: root.newAlbumMode ? newAlbumNameField.text.trim() : "",
                        newAlbumArtist: root.newAlbumMode ? newAlbumArtistField.text.trim() : "",
                        lyricsText: lyricsArea.text,
                        albumCoverPath: root.selectedCover(),
                        songCoverPath: root.selectedSongCover()
                    })
                    if (taskId.length > 0) { root.uploadQueued(); root.accept() }
                }
            }
        }
    }
    onOpened: { root.resetForm(); appController.refreshAlbums(); root.setAlbumChoices(appController.albums) }
    onRejected: root.resetForm()
    onAccepted: root.resetForm()
}
