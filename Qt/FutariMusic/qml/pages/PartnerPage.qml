import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Item {
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        Text { text: "我的搭子"; color: Theme.text; font.pixelSize: 32; font.bold: true }
        RowLayout {
            FutariTextField { id: keyword; placeholderText: "搜索用户名或昵称"; Layout.preferredWidth: 280; onAccepted: appController.searchUsers(text) }
            FutariButton { text: "搜索"; onClicked: appController.searchUsers(keyword.text) }
            FutariButton { text: "刷新"; onClicked: appController.refreshPartners() }
        }
        Text { text: "搜索结果"; color: Theme.text; font.pixelSize: 17; font.bold: true }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: 130; clip: true; model: appController.userResults
            delegate: RowLayout {
                width: ListView.view.width; height: 48
                Text { text: modelData.nickname + (modelData.username ? " · @" + modelData.username : ""); color: Theme.text; Layout.fillWidth: true }
                FutariButton {
                    text: modelData.username === appController.username ? "当前账户" : "添加搭子"
                    enabled: modelData.username !== appController.username
                    onClicked: appController.addPartner(modelData.id)
                }
            }
        }
        FutariButton { text: "更多搜索结果"; visible: appController.hasMoreUsers; onClicked: appController.loadMoreUsers() }
        Text { text: "已收藏的搭子"; color: Theme.text; font.pixelSize: 17; font.bold: true }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8; model: appController.partners
            delegate: Rectangle {
                width: ListView.view.width; height: 62; radius: 12; color: Theme.surface
                RowLayout { anchors.fill: parent; anchors.margins: 12
                    Text { text: (modelData.user ? modelData.user.nickname : "搭子") + (modelData.user && modelData.user.online ? " · 在线" : " · 离线"); color: Theme.text; Layout.fillWidth: true }
                    FutariButton { text: "邀请一起听"; enabled: !!appController.roomState.room; onClicked: appController.invitePartner(modelData.user.id) }
                    FutariButton { text: "移除"; onClicked: appController.removePartner(modelData.user.id) }
                }
            }
        }
    }
}
