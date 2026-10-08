import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    property int pendingDeleteId: 0
    property string pendingDeleteName: ""
    function confirmDelete(id, name) {
        root.pendingDeleteId = id
        root.pendingDeleteName = name
        deleteRoomDialog.open()
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        Text { text: "一起听房间"; color: Theme.text; font.pixelSize: 32; font.bold: true }
        Text { text: appController.roomState.room ? "当前房间：" + appController.roomState.room.name : "选一个房间，和搭子同步听歌"; color: Theme.muted }
        RowLayout {
            FutariTextField { id: roomName; placeholderText: "新房间名称"; Layout.preferredWidth: 240 }
            FutariButton { text: "创建房间"; onClicked: appController.createRoom(roomName.text) }
            FutariButton { text: "刷新房间"; onClicked: appController.refreshRooms() }
            FutariButton { text: "离开房间"; enabled: !!appController.roomState.room; onClicked: appController.leaveRoom() }
            FutariButton {
                objectName: "dissolveCurrentRoomButton"
                text: "解散房间"; visible: appController.roomOwner; enabled: !appController.roomDeletionBusy
                prominent: true; accent: Theme.danger; iconName: "remove"
                onClicked: root.confirmDelete(appController.roomState.room.id, appController.roomState.room.name)
            }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 20
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; radius: Theme.radius; color: Theme.surface
                Column {
                    anchors.fill: parent; anchors.margins: 18; spacing: 10
                    Text { text: "公开房间"; font.pixelSize: 19; font.bold: true; color: Theme.text }
                    ListView {
                        width: parent.width; height: parent.height - 85; clip: true; spacing: 8; model: appController.rooms
                        delegate: Rectangle {
                            width: ListView.view.width; height: 58; radius: 11; color: Theme.secondary
                            RowLayout { anchors.fill: parent; anchors.margins: 10
                                Column {
                                    Text { text: modelData.name; color: Theme.text; font.bold: true }
                                    Text { text: (modelData.memberCount || 0) + " 人在听"; color: Theme.muted; font.pixelSize: 12 }
                                }
                                Item { Layout.fillWidth: true }
                                FutariButton { text: "加入"; onClicked: appController.joinRoom(modelData.id) }
                                FutariToolButton {
                                    visible: appController.canDeleteRoom(modelData.id); enabled: !appController.roomDeletionBusy
                                    iconName: "remove"; ToolTip.visible: hovered; ToolTip.text: "解散此房间"
                                    onClicked: root.confirmDelete(modelData.id, modelData.name)
                                }
                            }
                        }
                    }
                    FutariButton { text: "加载更多房间"; visible: appController.hasMoreRooms; onClicked: appController.loadMoreRooms() }
                }
            }
            Rectangle {
                Layout.preferredWidth: 300; Layout.fillHeight: true; radius: Theme.radius; color: Theme.surface
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 18; spacing: 14
                    Text { text: "房间成员"; font.pixelSize: 19; font.bold: true; color: Theme.text }
                    Text { text: appController.canControl ? "你可以控制播放" : "房主尚未授予播放控制权"; color: Theme.accent; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ListView {
                        Layout.fillWidth: true; Layout.preferredHeight: Math.min(130, contentHeight); clip: true
                        model: appController.roomState.members || []
                        delegate: RowLayout {
                            width: ListView.view.width; height: 42
                            Text { text: modelData.nickname || modelData.username; color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight }
                            FutariButton {
                                visible: appController.roomOwner && modelData.id !== appController.roomState.room.ownerId
                                text: (appController.roomState.controllerIds || []).indexOf(modelData.id) >= 0 ? "撤销" : "授权"
                                onClicked: appController.setController(modelData.id, text === "授权")
                            }
                        }
                    }
                    QueueDrawer {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        roomQueue: true; showModePicker: false
                        visible: !!appController.roomState.room
                    }
                    Item { Layout.fillHeight: true; visible: !appController.roomState.room }
                }
            }
        }
    }
    FutariDialog {
        id: deleteRoomDialog; objectName: "deleteRoomDialog"
        title: "解散房间"; modal: true; anchors.centerIn: parent
        width: Math.min(root.width - 24, 420)
        implicitHeight: deleteContent.implicitHeight + topPadding + bottomPadding + 60
        standardButtons: Dialog.NoButton
        ColumnLayout {
            id: deleteContent
            anchors.fill: parent; spacing: 18
            Label {
                text: "确认解散「" + root.pendingDeleteName + "」？房间成员会退出，房间共享播放列表将被清除。"
                color: Theme.text; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                FutariButton { text: "取消"; onClicked: deleteRoomDialog.reject() }
                FutariButton {
                    text: "解散房间"; prominent: true; accent: Theme.danger; enabled: !appController.roomDeletionBusy
                    onClicked: { appController.deleteRoom(root.pendingDeleteId); deleteRoomDialog.accept() }
                }
            }
        }
    }
}
