import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Rectangle {
    id: root
    height: 100
    color: Theme.surface
    // 仅控制队列抽屉展示；播放、进度和加载状态始终绑定 player。
    property bool queueOpen: false
    signal openQueue()
    signal openLyrics()
    function formatTime(ms) {
        let seconds = Math.floor(Math.max(0, ms) / 1000)
        return Math.floor(seconds / 60).toString().padStart(2, "0") + ":" + (seconds % 60).toString().padStart(2, "0")
    }
    Rectangle { width: parent.width; height: 1; color: Theme.border }
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 22; anchors.rightMargin: 22; spacing: 16
        RowLayout {
            Layout.preferredWidth: 288
            spacing: 16
            Rectangle {
                Layout.preferredWidth: 62; Layout.preferredHeight: 62; radius: 11; color: Theme.secondary; clip: true
                SvgIcon { anchors.centerIn: parent; name: "music-note"; iconSize: 26; color: Theme.iconAccent }
                Image {
                    anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                    source: appController.covers[String(appController.player.song.id)] || ""
                    visible: status === Image.Ready
                }
                Rectangle {
                    anchors.fill: parent
                    color: "#79000000"
                    opacity: artHover.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.colorTransition } }
                    SvgIcon { anchors.centerIn: parent; name: "expand"; color: "white"; iconSize: 23 }
                }
                HoverHandler { id: artHover }
            }
            ColumnLayout {
                Layout.preferredWidth: 210
                Text { text: appController.player.song.title || "选择一首歌开始播放"; color: Theme.text; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                Text { text: appController.player.song.artist || "Futari Music"; color: Theme.muted; font.pixelSize: 12 }
            }
            HoverHandler { id: detailsHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: root.openLyrics() }
            ToolTip.visible: detailsHover.hovered
            ToolTip.text: "展开歌曲详情与歌词"
        }
        Item { Layout.fillWidth: true }
        ColumnLayout {
            Layout.preferredWidth: 410; spacing: 1
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                FutariToolButton { iconName: "previous"; enabled: !appController.roomState.room || appController.canControl; onClicked: appController.previousSong(); ToolTip.visible: hovered; ToolTip.text: "上一首" }
                FutariButton {
                    iconName: appController.player.playing ? "pause" : "play"
                    prominent: true; accent: Theme.accent; implicitWidth: 48; implicitHeight: 42
                    enabled: (!!appController.player.song.id || !!appController.player.song.localPath) && (!appController.roomState.room || appController.canControl)
                    onClicked: appController.togglePlayback()
                }
                FutariToolButton {
                    iconName: "next"; enabled: appController.roomState.room ? (appController.canControl && (appController.roomState.songIds || []).length > 0) : appController.localQueue.length > 0
                    onClicked: appController.nextSong()
                }
            }
            RowLayout {
                Text { text: root.formatTime(appController.player.position); color: Theme.muted; font.pixelSize: 11 }
                Slider { Layout.fillWidth: true; from: 0; to: Math.max(1, appController.player.duration); value: appController.player.position; enabled: !appController.roomState.room || appController.canControl; onMoved: appController.seek(value) }
                Text { text: root.formatTime(appController.player.duration); color: Theme.muted; font.pixelSize: 11 }
            }
        }
        Item { Layout.fillWidth: true }
        FutariToolButton { iconName: "lyrics"; onClicked: root.openLyrics(); ToolTip.visible: hovered; ToolTip.text: "歌词" }
        FutariToolButton { iconName: "queue"; onClicked: root.openQueue(); ToolTip.visible: hovered; ToolTip.text: "播放队列" }
        SvgIcon { name: "volume"; color: Theme.iconSecondary; iconSize: 18 }
        Slider {
            Layout.preferredWidth: 85
            from: 0
            to: 1
            value: appController.player.volume
            onMoved: appController.player.volume = value
        }
        Text {
            text: Math.round(appController.player.volume * 100) + "%"
            color: Theme.muted
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 34
        }
    }
}
