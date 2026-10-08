import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Rectangle {
    id: root
    Layout.preferredWidth: 222
    color: Theme.surface
    property string currentPage: "library"
    signal navigate(string page)

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 12
        Text { text: "Futari Music"; color: Theme.accent; font.pixelSize: 22; font.bold: true; Layout.bottomMargin: 12 }
        Flickable {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: width; contentHeight: navColumn.implicitHeight
            Column {
                id: navColumn; width: parent.width; spacing: 6
                Repeater {
                    model: [
                        { key: "library", label: "曲库", icon: "music-note" },
                        { key: "rooms", label: "一起听房间", icon: "room" },
                        { key: "partners", label: "搭子", icon: "heart" },
                        { key: "playlists", label: "我的歌单", icon: "playlist" },
                        { key: "serverPlaylists", label: "服务器歌单", icon: "playlist" },
                        { key: "lyrics", label: "歌词", icon: "lyrics" },
                        { key: "transfers", label: "上传与下载", icon: "download" }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        width: navColumn.width; height: 46; radius: 13
                        color: root.currentPage === modelData.key ? Theme.secondary : (navHover.hovered ? Theme.hover : "transparent")
                        HoverHandler { id: navHover }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter; x: 14; spacing: 14
                            SvgIcon { name: modelData.icon; iconSize: 19; color: root.currentPage === modelData.key ? Theme.iconAccent : Theme.iconSecondary }
                            Text { text: modelData.label; font.pixelSize: 14; color: Theme.text; anchors.verticalCenter: parent.verticalCenter }
                        }
                        TapHandler { onTapped: root.navigate(modelData.key) }
                    }
                }
            }
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            FutariButton {
                visible: appController.admin; Layout.fillWidth: true; text: "管理"; iconName: "admin"; iconSize: 16
                onClicked: { appController.refreshAdminUsers(); root.navigate("admin") }
            }
            FutariButton {
                Layout.fillWidth: true; text: "设置"; iconName: "settings"; iconSize: 16
                onClicked: root.navigate("settings")
            }
        }
    }
}
