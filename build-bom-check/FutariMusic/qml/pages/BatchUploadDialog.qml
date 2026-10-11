import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

FutariDialog {
    id: root
    title: "批量上传歌曲"
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent.width - 20, 1120)
    height: Math.min(parent.height - 20, 760)
    padding: 20
    property bool autoMetadata: true
    property bool autoLyrics: true
    property string pendingMetadataId: ""
    property var parseQueue: []
    property bool parsing: false
    property int selectedIndex: -1
    property int skippedDuplicateCount: 0
    property bool loadingSelection: false
    property bool applyingBulk: false
    property string bulkArtist: ""
    property string bulkAlbumName: ""
    property string bulkAlbumArtist: ""
    property string karaokeLyricName: ""
    property bool useSongDirectoryForLineLyrics: true
    property bool useSongDirectoryForKaraokeLyrics: true
    property bool syncingLyricsDirectoryModes: false
    signal uploadsQueued()

    ListModel { id: filesModel; objectName: "batchUploadFiles" }
    function formatBytes(bytes) {
        const value = Math.max(0, Number(bytes || 0))
        if (value < 1024) return value + " B"
        if (value < 1024 * 1024) return (value / 1024).toFixed(1) + " KB"
        return (value / (1024 * 1024)).toFixed(1) + " MB"
    }
    function formatDuration(durationMs) {
        const seconds = Math.max(0, Math.floor(Number(durationMs || 0) / 1000))
        const minutesText = Math.floor(seconds / 60).toString()
        const secondsText = (seconds % 60).toString()
        return (minutesText.length < 2 ? "0" : "") + minutesText + ":" + (secondsText.length < 2 ? "0" : "") + secondsText
    }
    function cleanUrl(url) { return String(url).replace(/\\/g, "/") }
    function findLyricsForAudio(url) {
        const match = appController.findLyricsForAudio(url)
        // Keep a canceled custom-folder choice explicit instead of falling back to the audio folder.
        if (!root.useSongDirectoryForLineLyrics && !appController.lineLyricsDirectory.length)
            match.line = { found: false }
        if (!root.useSongDirectoryForKaraokeLyrics && !appController.karaokeLyricsDirectory.length)
            match.karaoke = { found: false }
        return match
    }
    function setLyricsDirectoryModes(lineUsesSongDirectory, karaokeUsesSongDirectory) {
        root.syncingLyricsDirectoryModes = true
        root.useSongDirectoryForLineLyrics = lineUsesSongDirectory
        root.useSongDirectoryForKaraokeLyrics = karaokeUsesSongDirectory
        root.syncingLyricsDirectoryModes = false
    }
    function matchLyricsForAll() {
        for (let i = 0; i < filesModel.count; ++i) {
            const row = filesModel.get(i)
            if (row.lyricsText && row.karaokeLyricsText) continue
            const match = root.findLyricsForAudio(row.audioUrl)
            const line = match.line || { found: false }
            const karaoke = match.karaoke || { found: false }
            if (!row.lyricsText && line.found) {
                filesModel.setProperty(i, "lyricsText", line.text)
                filesModel.setProperty(i, "lyricsName", line.name)
            }
            if (!row.karaokeLyricsText && karaoke.found) {
                filesModel.setProperty(i, "karaokeLyricsText", karaoke.text)
                filesModel.setProperty(i, "karaokeLyricsName", karaoke.name)
            }
            const names = []
            if (filesModel.get(i).lyricsName) names.push("逐行：" + filesModel.get(i).lyricsName)
            if (filesModel.get(i).karaokeLyricsName) names.push("逐字：" + filesModel.get(i).karaokeLyricsName)
            filesModel.setProperty(i, "lyricStatus", names.length ? "已匹配 " + names.join(" · ") : "未找到同名歌词")
        }
        if (root.selectedIndex >= 0) root.selectFile(root.selectedIndex)
    }
    function findIndex(id) {
        for (let i = 0; i < filesModel.count; ++i) if (filesModel.get(i).id === id) return i
        return -1
    }
    function addFiles(urls) {
        let skipped = 0
        for (let n = 0; n < urls.length; ++n) {
            const url = cleanUrl(urls[n])
            let duplicate = false
            for (let i = 0; i < filesModel.count; ++i) if (filesModel.get(i).audioUrl === url) duplicate = true
            if (duplicate) { skipped++; continue }
            const fallback = appController.guessTrackMetadata(url)
            const parsedUrl = url.split("/")
            const fileName = decodeURIComponent(parsedUrl[parsedUrl.length - 1] || url)
            const requestId = Date.now().toString() + Math.random().toString().slice(2)
            const lyric = root.autoLyrics ? root.findLyricsForAudio(url) : {}
            const line = lyric.line || { found: false }
            const karaoke = lyric.karaoke || { found: false }
            filesModel.append({
                id: requestId, audioUrl: url, fileName: fileName,
                title: fallback.length ? fallback[0].title : fileName.replace(/\.[^.]+$/, ""),
                artist: fallback.length ? fallback[0].artist : "",
                albumName: "", albumArtist: "", albumId: 0, newAlbumName: "", newAlbumArtist: "", newAlbumMode: false,
                durationMs: 0, format: fileName.split(".").pop().toUpperCase(), fileSize: 0,
                coverUrl: "", lyricsText: line.found ? line.text : "", lyricsName: line.found ? line.name : "",
                karaokeLyricsText: karaoke.found ? karaoke.text : "", karaokeLyricsName: karaoke.found ? karaoke.name : "",
                metaStatus: "等待读取音频标签", lyricStatus: line.found || karaoke.found ? "已匹配歌词文件" : "未找到同名歌词",
                titleEdited: false, artistEdited: false, albumArtistEdited: false, duplicate: false, error: ""
            })
            if (root.autoMetadata) root.parseQueue.push({ id: requestId, url: url })
        }
        root.skippedDuplicateCount += skipped
        root.resolveAlbums()
        root.parseNext()
    }
    function parseNext() {
        if (root.parsing || !root.autoMetadata || root.parseQueue.length === 0) return
        const next = root.parseQueue.shift()
        root.parsing = true; root.pendingMetadataId = next.id
        appController.inspectAudio(next.url, next.id)
    }
    function resolveAlbums() {
        for (let i = 0; i < filesModel.count; ++i) {
            const row = filesModel.get(i)
            if (!row.albumName || row.newAlbumMode || row.albumId > 0) continue
            let matches = []
            for (let a = 0; a < appController.albums.length; ++a)
                if (String(appController.albums[a].name || "").toLocaleLowerCase() === row.albumName.toLocaleLowerCase()) matches.push(appController.albums[a])
            let chosen = null
            if (row.albumArtist) {
                const artistMatches = matches.filter(album => String(album.artist || "").toLocaleLowerCase() === row.albumArtist.toLocaleLowerCase())
                if (artistMatches.length === 1) chosen = artistMatches[0]
            } else if (matches.length === 1) chosen = matches[0]
            if (chosen) {
                filesModel.setProperty(i, "albumId", Number(chosen.id))
                filesModel.setProperty(i, "albumName", chosen.name)
                filesModel.setProperty(i, "albumArtist", chosen.artist || "")
                filesModel.setProperty(i, "metaStatus", "已匹配专辑")
            } else if (matches.length > 1) filesModel.setProperty(i, "metaStatus", "文件专辑已识别，曲库有多个同名项，请手动选择")
            else if (row.albumName) filesModel.setProperty(i, "metaStatus", "文件专辑已识别，但曲库暂无匹配项")
        }
    }
    function selectFile(index) {
        if (index < 0 || index >= filesModel.count) return
        root.loadingSelection = true
        root.selectedIndex = index
        const row = filesModel.get(index)
        titleField.text = row.title; artistField.text = row.artist
        albumArtistField.text = row.albumArtist
        root.lyricName = row.lyricsName
        lyricsArea.text = row.lyricsText
        root.karaokeLyricName = row.karaokeLyricsName
        karaokeLyricsArea.text = row.karaokeLyricsText
        newAlbumNameField.text = row.newAlbumName || row.albumName
        newAlbumArtistField.text = row.newAlbumArtist || row.albumArtist || row.artist
        coverUrlField.text = row.coverUrl
        newAlbumMode.checked = row.newAlbumMode
        let selected = 0
        for (let i = 0; i < appController.albums.length; ++i) if (Number(appController.albums[i].id) === Number(row.albumId)) selected = i + 1
        albumSelector.currentIndex = selected
        root.loadingSelection = false
    }
    function clearSelectedEditor() {
        // Reset the visible editor whenever its backing selection disappears.
        root.loadingSelection = true
        root.selectedIndex = -1
        titleField.text = ""
        artistField.text = ""
        albumArtistField.text = ""
        lyricsArea.text = ""
        karaokeLyricsArea.text = ""
        root.lyricName = ""
        root.karaokeLyricName = ""
        newAlbumNameField.text = ""
        newAlbumArtistField.text = ""
        coverUrlField.text = ""
        newAlbumMode.checked = false
        albumSelector.currentIndex = 0
        root.loadingSelection = false
    }
    function saveSelected() {
        if (root.loadingSelection) return
        const i = root.selectedIndex
        if (i < 0 || i >= filesModel.count) return
        filesModel.setProperty(i, "title", titleField.text)
        filesModel.setProperty(i, "artist", artistField.text)
        filesModel.setProperty(i, "lyricsText", lyricsArea.text)
        filesModel.setProperty(i, "lyricsName", root.lyricName)
        filesModel.setProperty(i, "karaokeLyricsText", karaokeLyricsArea.text)
        filesModel.setProperty(i, "karaokeLyricsName", root.karaokeLyricName)
        filesModel.setProperty(i, "coverUrl", coverUrlField.text)
        filesModel.setProperty(i, "newAlbumMode", newAlbumMode.checked)
        if (newAlbumMode.checked) {
            filesModel.setProperty(i, "albumId", 0)
            filesModel.setProperty(i, "newAlbumName", newAlbumNameField.text.trim())
            filesModel.setProperty(i, "newAlbumArtist", newAlbumArtistField.text.trim())
            filesModel.setProperty(i, "albumName", newAlbumNameField.text.trim())
            filesModel.setProperty(i, "albumArtist", newAlbumArtistField.text.trim())
        } else {
            const picked = albumSelector.currentIndex > 0 ? appController.albums[albumSelector.currentIndex - 1] : null
            filesModel.setProperty(i, "albumId", picked ? Number(picked.id) : 0)
            filesModel.setProperty(i, "newAlbumName", ""); filesModel.setProperty(i, "newAlbumArtist", "")
            filesModel.setProperty(i, "albumName", picked ? picked.name : "")
            filesModel.setProperty(i, "albumArtist", picked ? picked.artist || "" : albumArtistField.text)
        }
    }
    function removeFile(index) {
        if (index < 0 || index >= filesModel.count) return
        filesModel.remove(index)
        if (root.selectedIndex === index) root.clearSelectedEditor()
        else if (root.selectedIndex > index) root.selectedIndex--
    }
    function submitBatch() {
        if (root.parsing || root.parseQueue.length > 0) return
        root.saveSelected()
        let queued = 0
        const albumCoversQueued = ({})
        for (let i = 0; i < filesModel.count; ++i) {
            const row = filesModel.get(i)
            if (!row.title.trim()) { filesModel.setProperty(i, "error", "请填写歌曲名称"); continue }
            const albumIdentity = (row.newAlbumName + "\u001f" + row.newAlbumArtist).toLocaleLowerCase()
            const includeAlbumCover = row.newAlbumMode && row.coverUrl && !albumCoversQueued[albumIdentity]
            if (includeAlbumCover) albumCoversQueued[albumIdentity] = true
            const taskId = appController.enqueueUpload({
                audioUrl: row.audioUrl, title: row.title.trim(), artist: row.artist.trim(), durationMs: row.durationMs,
                album: row.albumName, albumId: row.newAlbumMode ? 0 : row.albumId,
                newAlbumName: row.newAlbumMode ? row.newAlbumName : "",
                newAlbumArtist: row.newAlbumMode ? row.newAlbumArtist : "",
                lyricsText: row.lyricsText,
                karaokeLyricsText: row.karaokeLyricsText,
                songCoverPath: row.coverUrl,
                albumCoverPath: includeAlbumCover ? row.coverUrl : ""
            })
            if (taskId.length > 0) queued++
            else filesModel.setProperty(i, "error", "无法创建上传任务，请检查音频文件")
        }
        if (queued > 0) { root.uploadsQueued(); root.accept() }
    }

    FileDialog {
        id: audioPicker; title: "选择多个音频文件"; fileMode: FileDialog.OpenFiles
        nameFilters: ["音频 (*.mp3 *.flac *.aac *.ogg *.wav *.m4a)"]
        onAccepted: root.addFiles(selectedFiles)
    }
    FileDialog {
        id: lyricsPicker; title: "选择歌词文件（可多选，按内容识别）"; fileMode: FileDialog.OpenFiles
        nameFilters: ["歌词 (*.lrc *.txt)"]
        onAccepted: {
            for (const selected of selectedFiles) {
                const url = selected.toString()
                const text = appController.readLyricsFile(url)
                const name = decodeURIComponent(url.split("/").pop())
                if (appController.classifyLyrics(text) === "karaoke") {
                    root.karaokeLyricName = name; karaokeLyricsArea.text = text
                } else {
                    root.lyricName = name; lyricsArea.text = text
                }
            }
            root.saveSelected()
        }
    }
    FolderDialog {
        id: lineLyricsFolderPicker; title: "选择逐行歌词目录"
        onAccepted: { appController.lineLyricsDirectory = selectedFolder.toString(); root.useSongDirectoryForLineLyrics = false; root.matchLyricsForAll() }
    }
    FolderDialog {
        id: karaokeLyricsFolderPicker; title: "选择逐字歌词目录"
        onAccepted: { appController.karaokeLyricsDirectory = selectedFolder.toString(); root.useSongDirectoryForKaraokeLyrics = false; root.matchLyricsForAll() }
    }
    FileDialog {
        id: coverPicker; title: "为当前歌曲选择封面"; nameFilters: ["图片 (*.jpg *.jpeg *.png *.webp)"]
        onAccepted: { coverUrlField.text = selectedFile.toString(); root.saveSelected() }
    }
    property string lyricName: ""

    Connections {
        target: appController
        function onAudioMetadataReady(requestId, metadata) {
            if (!root.parsing || requestId !== root.pendingMetadataId) return
            root.parsing = false
            const index = root.findIndex(requestId)
            if (index >= 0 && root.autoMetadata) {
                const fallback = appController.guessTrackMetadata(filesModel.get(index).audioUrl)
                if (!filesModel.get(index).titleEdited && metadata.title) filesModel.setProperty(index, "title", metadata.title)
                else if (!filesModel.get(index).titleEdited && fallback.length) filesModel.setProperty(index, "title", fallback[0].title)
                if (!filesModel.get(index).artistEdited && metadata.artist) filesModel.setProperty(index, "artist", metadata.artist)
                else if (!filesModel.get(index).artistEdited && fallback.length) filesModel.setProperty(index, "artist", fallback[0].artist)
                if (metadata.album) filesModel.setProperty(index, "albumName", metadata.album)
                if (!filesModel.get(index).albumArtistEdited)
                    filesModel.setProperty(index, "albumArtist", metadata.albumArtist || filesModel.get(index).artist)
                if (metadata.coverUrl) filesModel.setProperty(index, "coverUrl", metadata.coverUrl)
                filesModel.setProperty(index, "durationMs", Number(metadata.durationMs || 0))
                filesModel.setProperty(index, "fileSize", Number(metadata.fileSize || 0))
                filesModel.setProperty(index, "format", String(metadata.format || "").toUpperCase())
                filesModel.setProperty(index, "metaStatus", metadata.available ? "已读取音频标签" : "未读取标签，使用文件名")
                root.resolveAlbums()
                if (root.selectedIndex === index) root.selectFile(index)
            }
            root.parseNext()
        }
        function onAlbumsChanged() { root.resolveAlbums() }
        function onSettingsChanged() {
            root.useSongDirectoryForLineLyrics = appController.lineLyricsDirectory.length === 0
            root.useSongDirectoryForKaraokeLyrics = appController.karaokeLyricsDirectory.length === 0
            if (root.autoLyrics) root.matchLyricsForAll()
        }
    }
    Connections {
        target: root
        function onAutoMetadataChanged() {
            if (!root.autoMetadata) { root.parseQueue = []; root.parsing = false; return }
            for (let i = 0; i < filesModel.count; ++i) {
                if (filesModel.get(i).metaStatus === "等待读取音频标签") root.parseQueue.push({ id: filesModel.get(i).id, url: filesModel.get(i).audioUrl })
            }
            root.parseNext()
        }
    }

    ColumnLayout {
        anchors.fill: parent; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            FutariCheckBox { text: "根据元信息自动填充"; checked: root.autoMetadata; onToggled: root.autoMetadata = checked }
            FutariCheckBox { text: "自动查找歌词"; checked: root.autoLyrics; onToggled: root.autoLyrics = checked }
            Item { Layout.fillWidth: true }
            FutariButton { text: "选择多个音频"; iconName: "upload"; onClicked: audioPicker.open() }
            FutariButton { text: "清空列表"; enabled: filesModel.count > 0; onClicked: { filesModel.clear(); root.selectedIndex = -1; root.parseQueue = []; root.lyricName = ""; root.karaokeLyricName = ""; lyricsArea.text = ""; karaokeLyricsArea.text = "" } }
        }
        RowLayout {
            Layout.fillWidth: true
            FutariCheckBox {
                text: "逐行歌词使用歌曲目录"; checked: root.useSongDirectoryForLineLyrics
                onToggled: {
                    if (root.syncingLyricsDirectoryModes) return
                    if (checked) {
                        root.setLyricsDirectoryModes(true, false)
                        appController.lineLyricsDirectory = ""
                        root.matchLyricsForAll()
                    } else lineLyricsFolderPicker.open()
                }
            }
            FutariButton { visible: !root.useSongDirectoryForLineLyrics; text: appController.lineLyricsDirectory.length ? "更改逐行目录" : "选择逐行目录"; ToolTip.visible: hovered && appController.lineLyricsDirectory.length > 0; ToolTip.text: appController.lineLyricsDirectory; onClicked: lineLyricsFolderPicker.open() }
            FutariCheckBox {
                text: "逐字歌词使用歌曲目录"; checked: root.useSongDirectoryForKaraokeLyrics
                onToggled: {
                    if (root.syncingLyricsDirectoryModes) return
                    if (checked) {
                        root.setLyricsDirectoryModes(false, true)
                        appController.karaokeLyricsDirectory = ""
                        root.matchLyricsForAll()
                    } else karaokeLyricsFolderPicker.open()
                }
            }
            FutariButton { visible: !root.useSongDirectoryForKaraokeLyrics; text: appController.karaokeLyricsDirectory.length ? "更改逐字目录" : "选择逐字目录"; ToolTip.visible: hovered && appController.karaokeLyricsDirectory.length > 0; ToolTip.text: appController.karaokeLyricsDirectory; onClicked: karaokeLyricsFolderPicker.open() }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
            RowLayout {
                anchors.fill: parent; anchors.margins: 10; spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                    Text { text: "待上传歌曲 · " + filesModel.count; color: Theme.text; font.pixelSize: 18; font.bold: true }
                    ListView {
                        id: songsList; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 3
                        model: filesModel
                        delegate: Rectangle {
                            required property int index
                            required property string id
                            required property string title
                            required property string artist
                            required property string albumName
                            required property string format
                            required property int durationMs
                            required property int fileSize
                            required property string metaStatus
                            required property string lyricStatus
                            required property string error
                            width: ListView.view.width; height: 70; radius: Theme.radiusSmall
                            color: root.selectedIndex === index ? Theme.secondary : Theme.background
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 8; spacing: 10
                                Rectangle {
                                    Layout.preferredWidth: 48; Layout.preferredHeight: 48; radius: 8; color: Theme.secondary; clip: true
                                    SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 22; visible: image.status !== Image.Ready }
                                    Image { id: image; anchors.fill: parent; source: filesModel.get(index).coverUrl; fillMode: Image.PreserveAspectCrop }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 2
                                    Text { text: (index + 1) + ". " + (title || "未命名歌曲") + " · " + (artist || "未知歌手"); color: Theme.text; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: (albumName || "未关联专辑") + " · " + format + " · " + root.formatDuration(durationMs) + " · " + root.formatBytes(fileSize) + " · " + metaStatus; color: Theme.muted; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text {
                                        text: filesModel.get(index).lyricsName || filesModel.get(index).karaokeLyricsName
                                              ? [filesModel.get(index).lyricsName ? "逐行：" + filesModel.get(index).lyricsName : "",
                                                 filesModel.get(index).karaokeLyricsName ? "逐字：" + filesModel.get(index).karaokeLyricsName : ""].filter(Boolean).join(" · ")
                                              : "歌词：未匹配"
                                        color: Theme.muted; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    Text { visible: error.length > 0; text: error; color: Theme.danger; font.pixelSize: 11 }
                                }
                                FutariToolButton { iconName: "more"; ToolTip.visible: hovered; ToolTip.text: "编辑歌曲"; onClicked: root.selectFile(index) }
                                FutariToolButton { iconName: "remove"; ToolTip.visible: hovered; ToolTip.text: "移除"; onClicked: root.removeFile(index) }
                            }
                            TapHandler { onTapped: root.selectFile(index) }
                        }
                        ScrollBar.vertical: ScrollBar {}
                    }
                }
                Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }
                Flickable {
                    Layout.preferredWidth: 335; Layout.fillHeight: true; clip: true
                    contentWidth: width; contentHeight: editor.implicitHeight
                    ColumnLayout {
                        id: editor; width: parent.width; spacing: 9
                        Text { text: root.selectedIndex >= 0 ? "编辑歌曲" : "选择一首歌曲编辑"; color: Theme.text; font.pixelSize: 17; font.bold: true }
                        FutariTextField { id: titleField; enabled: root.selectedIndex >= 0; placeholderText: "歌曲名称"; Layout.fillWidth: true; onTextEdited: { if (root.selectedIndex >= 0) filesModel.setProperty(root.selectedIndex, "titleEdited", true); root.saveSelected() } }
                        FutariTextField { id: artistField; enabled: root.selectedIndex >= 0; placeholderText: "歌手"; Layout.fillWidth: true; onTextEdited: { if (root.selectedIndex >= 0) filesModel.setProperty(root.selectedIndex, "artistEdited", true); root.saveSelected() } }
                        FutariTextField { id: albumArtistField; enabled: root.selectedIndex >= 0; placeholderText: "专辑艺术家"; Layout.fillWidth: true; onTextEdited: { if (root.selectedIndex >= 0) filesModel.setProperty(root.selectedIndex, "albumArtistEdited", true); root.saveSelected() } }
                        FutariCheckBox {
                            id: newAlbumMode; enabled: root.selectedIndex >= 0; text: "创建新专辑"
                            onToggled: root.saveSelected()
                        }
                        FutariComboBox {
                            id: albumSelector; enabled: root.selectedIndex >= 0 && !newAlbumMode.checked; Layout.fillWidth: true
                            model: [{ id: 0, name: "不关联专辑", artist: "" }].concat(appController.albums.map(album => ({ id: album.id, name: album.name, artist: album.artist, displayName: album.name + (album.artist ? " · " + album.artist : "") })))
                            textRole: "displayName"
                            onActivated: root.saveSelected()
                        }
                        FutariTextField { id: newAlbumNameField; enabled: root.selectedIndex >= 0 && newAlbumMode.checked; placeholderText: "新专辑名称"; Layout.fillWidth: true; onTextEdited: root.saveSelected() }
                        FutariTextField { id: newAlbumArtistField; enabled: root.selectedIndex >= 0 && newAlbumMode.checked; placeholderText: "专辑艺术家"; Layout.fillWidth: true; onTextEdited: { if (root.selectedIndex >= 0) filesModel.setProperty(root.selectedIndex, "albumArtistEdited", true); root.saveSelected() } }
                        FutariButton { text: coverUrlField.text ? "更换封面图片" : "选择封面图片"; enabled: root.selectedIndex >= 0; onClicked: coverPicker.open() }
                        FutariTextField { id: coverUrlField; visible: false; onTextChanged: root.saveSelected() }
                        RowLayout {
                            FutariButton { text: "选择歌词文件（自动识别）"; enabled: root.selectedIndex >= 0; onClicked: lyricsPicker.open() }
                            FutariButton { text: "清空逐行"; enabled: root.selectedIndex >= 0 && lyricsArea.text.length > 0; onClicked: { root.lyricName = ""; lyricsArea.text = ""; root.saveSelected() } }
                        }
                        TextArea {
                            id: lyricsArea; objectName: "batchLyricsEditor"; enabled: root.selectedIndex >= 0; Layout.fillWidth: true; Layout.preferredHeight: 140
                            placeholderText: "逐行歌词文本"; wrapMode: TextEdit.Wrap; color: Theme.text; placeholderTextColor: Theme.muted
                            background: Rectangle { radius: Theme.radiusSmall; color: Theme.fieldFill; border.color: lyricsArea.activeFocus ? Theme.fieldFocusBorder : Theme.fieldBorder }
                            onTextChanged: root.saveSelected()
                        }
                        RowLayout {
                            Text { text: root.karaokeLyricName || "逐字文本可由上方歌词文件选择器载入"; color: Theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            FutariButton { text: "清空逐字"; enabled: root.selectedIndex >= 0 && karaokeLyricsArea.text.length > 0; onClicked: { root.karaokeLyricName = ""; karaokeLyricsArea.text = ""; root.saveSelected() } }
                        }
                        TextArea {
                            id: karaokeLyricsArea; objectName: "batchKaraokeLyricsEditor"; enabled: root.selectedIndex >= 0; Layout.fillWidth: true; Layout.preferredHeight: 140
                            placeholderText: "逐字歌词（含 <分:秒.毫秒> 时间标签）"; wrapMode: TextEdit.Wrap; color: Theme.text; placeholderTextColor: Theme.muted
                            background: Rectangle { radius: Theme.radiusSmall; color: Theme.fieldFill; border.color: karaokeLyricsArea.activeFocus ? Theme.fieldFocusBorder : Theme.fieldBorder }
                            onTextChanged: root.saveSelected()
                        }
                        Text {
                            visible: root.selectedIndex >= 0 && appController.countLikelyCreditLines(lyricsArea.text + "\n" + karaokeLyricsArea.text) > 0
                            text: "检测到可能的演职员信息行，请预览确认；歌词仍可直接编辑。"
                            color: Theme.muted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true
                        }
                        FutariButton {
                            text: "将当前歌手和专辑应用到全部"
                            visible: filesModel.count > 1 && root.selectedIndex >= 0
                            onClicked: { root.saveSelected(); root.bulkArtist = artistField.text; root.bulkAlbumName = newAlbumMode.checked ? newAlbumNameField.text : (albumSelector.currentIndex > 0 ? albumSelector.currentText.split(" · ")[0] : ""); root.bulkAlbumArtist = newAlbumArtistField.text; bulkConfirm.open() }
                        }
                    }
                }
            }
        }
        DropArea {
            id: dropArea
            Layout.fillWidth: true; Layout.preferredHeight: 48; keys: ["text/uri-list"]
            Rectangle { anchors.fill: parent; radius: Theme.radius; color: dropArea.containsDrag ? Theme.hover : Theme.secondary; border.color: dropArea.containsDrag ? Theme.accent : Theme.border }
            Text { anchors.centerIn: parent; text: "将音频文件拖到此处添加到待上传列表"; color: Theme.muted }
            onDropped: drop => root.addFiles(drop.urls)
        }
        RowLayout {
            Layout.fillWidth: true
            Text { text: root.parsing ? "正在读取音频标签…" : (root.skippedDuplicateCount > 0 ? "已跳过重复选择的文件 " + root.skippedDuplicateCount + " 个 · 确认后才进入上传队列" : "文件先预处理并确认，确认后才会进入上传队列"); color: Theme.muted; Layout.fillWidth: true }
            FutariButton { text: "关闭"; onClicked: root.reject() }
            FutariButton { text: "确认批量上传（" + filesModel.count + "）"; prominent: true; enabled: filesModel.count > 0 && !root.parsing && root.parseQueue.length === 0; onClicked: batchConfirm.open() }
        }
    }

    FutariDialog {
        id: batchConfirm; title: "确认批量上传"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: "确认将有效的 " + filesModel.count + " 首歌曲加入上传队列？无效项目会保留在列表中并显示原因。"; color: Theme.text; wrapMode: Text.Wrap; width: 360 }
        onAccepted: root.submitBatch()
    }
    FutariDialog {
        id: bulkConfirm; title: "批量应用信息"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: "将当前歌手和专辑应用到全部歌曲，覆盖这些歌曲已有的对应字段？"; color: Theme.text; wrapMode: Text.Wrap; width: 360 }
        onAccepted: {
            for (let i = 0; i < filesModel.count; ++i) {
                filesModel.setProperty(i, "artist", root.bulkArtist)
                if (root.bulkAlbumName) {
                    filesModel.setProperty(i, "albumName", root.bulkAlbumName)
                    filesModel.setProperty(i, "albumArtist", root.bulkAlbumArtist)
                    filesModel.setProperty(i, "albumId", 0)
                    filesModel.setProperty(i, "newAlbumName", root.bulkAlbumName)
                    filesModel.setProperty(i, "newAlbumArtist", root.bulkAlbumArtist)
                    filesModel.setProperty(i, "newAlbumMode", true)
                }
            }
        }
    }
    Connections {
        target: root
        function onAutoLyricsChanged() {
            if (!root.autoLyrics) return
            root.matchLyricsForAll()
        }
    }
    onOpened: {
        filesModel.clear(); root.clearSelectedEditor(); root.parseQueue = []; root.skippedDuplicateCount = 0
        let lineUsesSongDirectory = appController.lineLyricsDirectory.length === 0
        let karaokeUsesSongDirectory = appController.karaokeLyricsDirectory.length === 0
        if (lineUsesSongDirectory && karaokeUsesSongDirectory) {
            // Keep the last selected audio-folder source when both directory preferences are empty.
            lineUsesSongDirectory = !root.useSongDirectoryForKaraokeLyrics
            karaokeUsesSongDirectory = root.useSongDirectoryForKaraokeLyrics && !root.useSongDirectoryForLineLyrics
            if (!lineUsesSongDirectory && !karaokeUsesSongDirectory) lineUsesSongDirectory = true
        }
        root.setLyricsDirectoryModes(lineUsesSongDirectory, karaokeUsesSongDirectory)
        if (root.useSongDirectoryForLineLyrics && root.useSongDirectoryForKaraokeLyrics)
            root.useSongDirectoryForKaraokeLyrics = false
        appController.refreshAlbums(); root.parseNext()
    }
    onRejected: { root.parseQueue = []; root.parsing = false; root.pendingMetadataId = "" }
}
