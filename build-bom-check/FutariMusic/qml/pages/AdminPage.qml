import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    Component.onCompleted: if (appController.admin) appController.refreshAdminUsers()
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        RowLayout {
            Text { text: "用户管理"; color: Theme.text; font.pixelSize: 32; font.bold: true }
            Item { Layout.fillWidth: true }
            FutariButton { text: "刷新"; onClicked: appController.refreshAdminUsers() }
        }
        Text { text: "管理员默认拥有上传和服务器歌单管理权限；可调整普通用户权限或强制重新登录。"; color: Theme.muted }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8; model: appController.adminUsers
            delegate: Rectangle {
                width: ListView.view.width; height: 65; radius: 12; color: Theme.surface
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 12
                    Text { text: modelData.username + " · " + modelData.role; color: Theme.text; Layout.fillWidth: true }
                    FutariComboBox {
                        model: ["USER", "ADMIN"]
                        currentIndex: modelData.role === "ADMIN" ? 1 : 0
                        onActivated: appController.updateRole(modelData.id, currentText)
                    }
                    FutariCheckBox {
                        text: "上传"
                        checked: modelData.role === "ADMIN" || modelData.canUpload
                        enabled: modelData.role !== "ADMIN"
                        onClicked: appController.updatePermissions(modelData.id, checked, modelData.canManageServerPlaylist)
                        ToolTip.visible: hovered && modelData.role === "ADMIN"
                        ToolTip.text: "管理员默认拥有上传权限"
                    }
                    FutariCheckBox {
                        text: "管理服务器歌单"
                        checked: modelData.role === "ADMIN" || modelData.canManageServerPlaylist
                        enabled: modelData.role !== "ADMIN"
                        onClicked: appController.updatePermissions(modelData.id, modelData.canUpload, checked)
                        ToolTip.visible: hovered && modelData.role === "ADMIN"
                        ToolTip.text: "管理员默认拥有服务器歌单管理权限"
                    }
                    FutariButton { text: "重置密码"; onClicked: passwordDialog.open() }
                    FutariButton { text: "强制下线"; onClicked: kickDialog.open() }
                }
                FutariDialog {
                    id: passwordDialog; title: "重置密码"; modal: true; parent: Overlay.overlay; anchors.centerIn: parent
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    onAccepted: { appController.resetPassword(modelData.id, newPassword.text); newPassword.text = "" }
                    FutariTextField { id: newPassword; placeholderText: "新密码，至少 8 位"; echoMode: TextInput.Password; width: 260 }
                }
                FutariDialog {
                    id: kickDialog; title: "确认强制下线？"; modal: true; parent: Overlay.overlay; anchors.centerIn: parent
                    standardButtons: Dialog.Yes | Dialog.No
                    onAccepted: appController.kickUser(modelData.id)
                }
            }
        }
        FutariButton { text: "加载更多用户"; visible: appController.hasMoreAdminUsers; Layout.alignment: Qt.AlignHCenter; onClicked: appController.loadMoreAdminUsers() }
    }
}
