import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    property bool karaokeEnabled: appController.lyricsDisplayMode === "automatic" &&
                                  String(appController.lyrics.karaokeLyrics || "").length > 0
    // Automatic mode prefers timed characters but keeps legacy songs readable from lyrics.
    property real lyricPosition: appController.player.position
    property double lyricSyncTime: Date.now()
    property string displaySource: karaokeEnabled
                                   ? appController.lyrics.karaokeLyrics
                                   : (appController.lyrics.lyrics || appController.lyrics.karaokeLyrics || "")
    property var lyricLines: appController.parseLyrics(displaySource)
    property int activeLine: {
        let active = -1
        for (let i = 0; i < lyricLines.length; ++i)
            if (Number(lyricLines[i].at) >= 0 && Number(lyricLines[i].at) <= lyricPosition) active = i
        return active
    }

    FrameAnimation {
        running: root.karaokeEnabled && appController.player.playing
        onTriggered: {
            const elapsed = Date.now() - root.lyricSyncTime
            const projected = appController.player.position + Math.max(0, elapsed)
            root.lyricPosition = appController.player.duration > 0
                                 ? Math.min(projected, appController.player.duration)
                                 : projected
        }
    }
    Connections {
        target: appController.player
        function onPositionChanged() {
            root.lyricPosition = appController.player.position
            root.lyricSyncTime = Date.now()
        }
        function onPlayingChanged() {
            root.lyricPosition = appController.player.position
            root.lyricSyncTime = Date.now()
        }
        function onSongChanged() {
            root.lyricPosition = appController.player.position
            root.lyricSyncTime = Date.now()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 45
        spacing: 46
        Rectangle {
            Layout.preferredWidth: Math.min(330, root.width * 0.36)
            Layout.preferredHeight: width
            radius: 26
            color: Theme.secondary
            clip: true
            SvgIcon {
                anchors.centerIn: parent
                name: "music-note"
                color: Theme.iconAccent
                iconSize: 110
            }
            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: appController.covers[String(appController.player.song.id)] || ""
                visible: status === Image.Ready
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: appController.player.song.title || "还没有播放歌曲"
                        color: Theme.text
                        font.pixelSize: 28
                        font.bold: true
                    }
                    Text {
                        text: appController.player.song.artist || ""
                        color: Theme.muted
                        font.pixelSize: 16
                    }
                }
            }
            Text {
                visible: root.lyricLines.length === 0
                text: "暂无歌词"
                color: Theme.muted
                font.pixelSize: 18
            }
            ListView {
                id: linesView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 12
                model: root.lyricLines
                currentIndex: root.activeLine
                preferredHighlightBegin: height * 0.35
                preferredHighlightEnd: height * 0.65
                highlightRangeMode: ListView.ApplyRange
                delegate: Button {
                    id: lyricRow
                    required property int index
                    required property var modelData
                    width: ListView.view.width
                    property bool current: index === root.activeLine
                    property bool karaokeVisible: root.karaokeEnabled && current && modelData.karaoke &&
                                                  !modelData.credit && modelData.characters.length > 0
                    property bool canSeekLine: Number(modelData.at) >= 0 &&
                                               (!appController.roomState.room || appController.canControl)
                    property int lyricFontSize: current ? 23 : 19
                    padding: 0
                    hoverEnabled: true
                    focusPolicy: Qt.TabFocus
                    enabled: canSeekLine
                    implicitHeight: contentItem.implicitHeight
                    Accessible.name: modelData.text
                    Accessible.description: canSeekLine
                                               ? "跳转到 " + Math.floor(Number(modelData.at) / 1000) + " 秒"
                                               : "此行当前不可定位"

                    onClicked: appController.seek(Number(modelData.at))

                    background: Rectangle {
                        color: lyricRow.down ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        border.width: lyricRow.activeFocus ? 1 : 0
                        border.color: Theme.accent
                    }

                    HoverHandler {
                        cursorShape: lyricRow.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }

                    contentItem: Item {
                        implicitWidth: lyricRow.width
                        implicitHeight: lyricRow.karaokeVisible
                                        ? karaokeText.implicitHeight : lineText.implicitHeight

                        Text {
                            id: lineText
                            anchors.left: parent.left
                            anchors.right: parent.right
                            visible: !lyricRow.karaokeVisible
                            text: lyricRow.modelData.text
                            color: lyricRow.modelData.credit ? Theme.muted
                                  : lyricRow.current ? Theme.accent
                                  : lyricRow.canSeekLine && lyricRow.hovered ? Theme.muted : Theme.text
                            opacity: root.activeLine < 0 || Math.abs(lyricRow.index - root.activeLine) <= 2
                                     ? 1 : 0.55
                            font.pixelSize: lyricRow.lyricFontSize
                            font.bold: lyricRow.current
                            wrapMode: Text.Wrap
                        }

                        KaraokeLine {
                            id: karaokeText
                            width: parent.width
                            visible: lyricRow.karaokeVisible
                            characters: lyricRow.modelData.characters
                            positionMs: root.lyricPosition
                            fontPixelSize: lyricRow.lyricFontSize
                            hovered: lyricRow.canSeekLine && lyricRow.hovered
                        }
                    }
                }
            }
        }
    }
}
