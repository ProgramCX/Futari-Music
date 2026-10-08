import QtQuick
import QtQuick.Layouts
import FutariMusic

Item {
    id: root
    function parseLyrics(value) {
        if (!value) return []
        let result = []
        let lines = value.split(/\r?\n/)
        for (let line of lines) {
            let match = line.match(/^\[(\d+):(\d+(?:\.\d+)?)\](.*)$/)
            if (match) result.push({ at: (Number(match[1]) * 60 + Number(match[2])) * 1000, text: match[3] || " " })
            else if (line.trim().length) result.push({ at: -1, text: line })
        }
        return result
    }
    property var lyricLines: parseLyrics(appController.lyrics.lyrics)
    property int activeLine: {
        let active = -1
        for (let i = 0; i < lyricLines.length; ++i)
            if (lyricLines[i].at >= 0 && lyricLines[i].at <= appController.player.position) active = i
        return active
    }
    RowLayout {
        anchors.fill: parent; anchors.margins: 45; spacing: 46
        Rectangle {
            Layout.preferredWidth: Math.min(330, root.width * 0.36); Layout.preferredHeight: width
            radius: 26; color: Theme.secondary; clip: true
            SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 110 }
            Image {
                anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                source: appController.covers[String(appController.player.song.id)] || ""
                visible: status === Image.Ready
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
            Text { text: appController.player.song.title || "还没有播放歌曲"; color: Theme.text; font.pixelSize: 28; font.bold: true }
            Text { text: appController.player.song.artist || ""; color: Theme.muted; font.pixelSize: 16 }
            Text { visible: root.lyricLines.length === 0; text: "暂无歌词"; color: Theme.muted; font.pixelSize: 18 }
            ListView {
                id: linesView
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 12
                model: root.lyricLines
                currentIndex: root.activeLine
                preferredHighlightBegin: height * 0.35; preferredHighlightEnd: height * 0.65
                highlightRangeMode: ListView.ApplyRange
                delegate: Text {
                    width: ListView.view.width
                    text: modelData.text
                    color: index === root.activeLine ? Theme.accent : Theme.text
                    opacity: root.activeLine < 0 || Math.abs(index - root.activeLine) <= 2 ? 1 : 0.55
                    font.pixelSize: index === root.activeLine ? 23 : 19
                    font.bold: index === root.activeLine
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
