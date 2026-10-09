import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic

Item {
    id: root
    property bool registering: false
    Rectangle {
        anchors.fill: parent
        color: Theme.authBackground
    }
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 430)
        height: Math.min(parent.height - 32, registering ? 620 : 550)
        radius: Theme.radiusLarge
        color: Theme.authSurface
        ColumnLayout {
            anchors.fill: parent; anchors.leftMargin: 30; anchors.rightMargin: 30
            anchors.topMargin: 28; anchors.bottomMargin: 22; spacing: 13
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 72; Layout.preferredHeight: 72
                radius: 20; color: Theme.secondary
                SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.authAccent; iconSize: 40 }
            }
            Text { text: "Futari Music"; color: Theme.authText; font.pixelSize: 27; font.bold: true; Layout.alignment: Qt.AlignHCenter }
            Text { text: registering ? "创建你的账号" : "欢迎回来，一起听歌"; color: Theme.authMuted; Layout.alignment: Qt.AlignHCenter; bottomPadding: 8 }
            FutariTextField {
                id: server; authStyle: true; Layout.fillWidth: true; text: appController.serverUrl; placeholderText: "服务器地址"; onEditingFinished: appController.serverUrl = text
            }
            FutariTextField {
                id: user; authStyle: true; Layout.fillWidth: true; text: appController.username; placeholderText: "用户名"
            }
            FutariTextField {
                id: nickname; authStyle: true; visible: registering; Layout.fillWidth: true; placeholderText: "昵称"
            }
            FutariTextField {
                id: password; authStyle: true; Layout.fillWidth: true; placeholderText: "密码"; echoMode: TextInput.Password; onAccepted: submit()
            }
            RowLayout {
                CheckBox {
                    id: autoLoginCheck
                    text: "自动登录（仅保存登录令牌）"
                    checked: appController.autoLogin
                    onClicked: appController.autoLogin = checked
                    spacing: 10
                    indicator: Rectangle {
                        implicitWidth: 20; implicitHeight: 20
                        x: autoLoginCheck.leftPadding
                        y: (autoLoginCheck.height - height) / 2
                        radius: 5
                        color: autoLoginCheck.checked ? Theme.authAccent : Theme.authField
                        border.width: 1
                        border.color: autoLoginCheck.checked ? Theme.authAccent : Theme.authBorder
                        SvgIcon {
                            anchors.centerIn: parent
                            name: "check"
                            color: Theme.authBackground
                            iconSize: 15
                            visible: autoLoginCheck.checked
                        }
                    }
                    contentItem: Text {
                        text: autoLoginCheck.text
                        color: autoLoginCheck.down ? Theme.authText : Theme.authMuted
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: autoLoginCheck.indicator.width + autoLoginCheck.spacing
                    }
                }
            }
            Text { text: appController.errorMessage; color: Theme.danger; visible: text.length > 0; wrapMode: Text.Wrap; Layout.fillWidth: true }
            FutariButton {
                id: loginButton
                text: registering ? "注册并登录" : "登录"
                Layout.fillWidth: true; Layout.preferredHeight: 46
                onClicked: submit()
                prominent: true; accent: Theme.authAction
                contentItem: Text { text: loginButton.text; color: Theme.accentText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.bold: true }
            }
            Item { Layout.fillHeight: true }
            Item {
                id: switchButton
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: switchLabel.implicitWidth + 32
                Layout.preferredHeight: 38
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: registering ? "返回登录" : "注册账号"
                HoverHandler { id: switchHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: { root.registering = !root.registering; appController.clearError() } }
                Keys.onReturnPressed: { root.registering = !root.registering; appController.clearError() }
                Keys.onSpacePressed: { root.registering = !root.registering; appController.clearError() }
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text {
                        text: root.registering ? "已有账号？" : "还没有账号？"
                        color: Theme.authMuted
                        font.pixelSize: 13
                    }
                    Text {
                        id: switchLabel
                        text: root.registering ? "返回登录" : "注册账号"
                        color: switchHover.hovered || switchButton.activeFocus
                               ? Qt.darker(Theme.authLink, 1.12) : Theme.authLink
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        font.underline: switchHover.hovered || switchButton.activeFocus
                    }
                }
            }
        }
    }
    function submit() {
        appController.serverUrl = server.text
        if (registering) appController.registerAccount(user.text, nickname.text, password.text)
        else appController.login(user.text, password.text)
    }
}
