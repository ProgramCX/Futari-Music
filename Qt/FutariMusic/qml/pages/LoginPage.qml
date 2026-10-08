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
        anchors.fill: parent
        color: Theme.authSurface
        ColumnLayout {
            anchors.fill: parent; anchors.leftMargin: 42; anchors.rightMargin: 42
            anchors.topMargin: 24; anchors.bottomMargin: 18; spacing: 12
            SvgIcon { name: "music-note"; color: Theme.authAccent; iconSize: 64; Layout.alignment: Qt.AlignHCenter }
            Text { text: "Futari Music"; color: Theme.authText; font.pixelSize: 27; font.bold: true; Layout.alignment: Qt.AlignHCenter }
            Text { text: registering ? "创建你的账号" : "欢迎回来，一起听歌"; color: Theme.authMuted; Layout.alignment: Qt.AlignHCenter; bottomPadding: 10 }
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
            Text { text: appController.errorMessage; color: "#ffadb3"; visible: text.length > 0; wrapMode: Text.Wrap; Layout.fillWidth: true }
            FutariButton {
                id: loginButton
                text: registering ? "注册并登录" : "登录"
                Layout.fillWidth: true; Layout.preferredHeight: 46
                onClicked: submit()
                prominent: true; accent: Theme.authAction
                contentItem: Text { text: loginButton.text; color: Theme.authText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.bold: true }
            }
            Item { Layout.fillHeight: true }
            FutariButton {
                id: switchButton
                text: registering ? "已有账号？返回登录" : "注册账号"; flat: true; Layout.alignment: Qt.AlignHCenter
                onClicked: { registering = !registering; appController.clearError() }
                contentItem: Text { text: switchButton.text; color: Theme.authLink; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
    function submit() {
        appController.serverUrl = server.text
        if (registering) appController.registerAccount(user.text, nickname.text, password.text)
        else appController.login(user.text, password.text)
    }
}
