import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Page {
    background: Rectangle { color: Theme.background }
    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 36
        spacing: 22

        Text { text: "账户"; color: Theme.text; font.pixelSize: 30; font.weight: Font.DemiBold }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 116
            radius: Theme.radiusLarge
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                anchors.fill: parent; anchors.margins: 22; spacing: 16
                Rectangle {
                    Layout.preferredWidth: 64; Layout.preferredHeight: 64; radius: 32
                    color: Theme.secondary
                    SvgIcon { anchors.centerIn: parent; name: "account"; iconSize: 28; color: Theme.iconAccent }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Text { text: appController.username; color: Theme.text; font.pixelSize: 20; font.weight: Font.Medium }
                    Text { text: appController.admin ? "管理员" : "普通用户"; color: Theme.muted; font.pixelSize: 14 }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 76
            radius: Theme.radiusLarge
            color: Theme.surface
            border.color: Theme.border
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 22; anchors.rightMargin: 22
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 4
                    Text { text: "外观"; color: Theme.text; font.pixelSize: 16 }
                    Text { text: Theme.dark ? "深色主题" : "浅色主题"; color: Theme.muted; font.pixelSize: 13 }
                }
                Switch {
                    id: themeSwitch
                    checked: Theme.dark
                    onToggled: appController.darkMode = checked
                    indicator: Rectangle {
                        implicitWidth: 48; implicitHeight: 28; radius: 14
                        color: themeSwitch.checked ? Theme.accent : Theme.border
                        Rectangle {
                            width: 22; height: 22; radius: 11; y: 3
                            x: themeSwitch.checked ? parent.width - width - 3 : 3
                            color: "white"
                            Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        }
                    }
                }
            }
        }
        FutariButton {
            text: "退出登录"
            iconName: "logout"
            onClicked: appController.logout()
        }
    }
}
