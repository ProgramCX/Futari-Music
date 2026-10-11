import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Item {
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        RowLayout { Layout.fillWidth: true
            Text { text: "消息"; color: Theme.text; font.pixelSize: 32; font.bold: true }
            Item { Layout.fillWidth: true }
            FutariButton { text: "刷新"; onClicked: appController.refreshInvites() }
        }
        Text { text: "一起听邀请 · " + appController.unreadInvites; color: Theme.muted }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 12
            model: appController.invites
            delegate: Rectangle {
                width: ListView.view.width; height: 106; radius: Theme.radius; color: Theme.surface
                RowLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 14
                    Rectangle { Layout.preferredWidth: 48; Layout.preferredHeight: 48; radius: 24; color: Theme.secondary; SvgIcon { anchors.centerIn: parent; name: "room"; color: Theme.iconAccent } }
                    Column { spacing: 5; Layout.fillWidth: true
                        Text { text: (modelData.from ? modelData.from.nickname : "搭子") + " 邀请你一起听"; color: Theme.text; font.pixelSize: 16; font.bold: true }
                        Text { text: modelData.roomName + (modelData.currentSongName ? " · " + modelData.currentSongName : ""); color: Theme.muted; elide: Text.ElideRight; width: parent.width }
                    }
                    FutariButton { text: "拒绝"; onClicked: appController.respondInvite(modelData.inviteId, false) }
                    FutariButton { text: "同意"; onClicked: appController.respondInvite(modelData.inviteId, true) }
                }
            }
        }
        Text { visible: appController.unreadInvites === 0; text: "暂无待处理邀请"; color: Theme.muted; Layout.alignment: Qt.AlignHCenter }
    }
}
