import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import FutariMusic
import "components"
import "pages"

ApplicationWindow {
    id: window
    width: 500; height: 740; minimumWidth: appController.authenticated ? 950 : 480; minimumHeight: appController.authenticated ? 640 : 650
    visible: true; title: "Futari Music"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    background: Item {}
    palette.window: Theme.background
    palette.base: Theme.surface
    palette.text: Theme.text
    palette.button: Theme.secondary
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    property string currentPage: "library"
    property var editingSong: ({})
    property bool queueVisible: false
    property var backStack: []
    property var forwardStack: []
    property bool toastOpensMessages: false
    property bool roomClosePending: false
    property bool skipRoomLeaveOnce: false
    Component.onCompleted: if (appController.authenticated) { width = 1320; height = 860 }
    onClosing: function(close) {
        if (window.skipRoomLeaveOnce) { window.skipRoomLeaveOnce = false; return }
        if (window.roomClosePending) { close.accepted = false; return }
        if (appController.roomState.room) {
            close.accepted = false
            window.roomClosePending = true
            roomLeaveTimeout.start()
            appController.leaveRoomBeforeClose()
        }
    }
    function finishClosing() {
        if (!window.roomClosePending) return
        window.roomClosePending = false
        window.skipRoomLeaveOnce = true
        roomLeaveTimeout.stop()
        window.close()
    }
    Timer { id: roomLeaveTimeout; interval: 3000; onTriggered: window.finishClosing() }
    function navigate(page) {
        if (!page || page === currentPage) return
        const back = backStack.slice(); back.push(currentPage)
        if (back.length > 80) back.shift()
        backStack = back; forwardStack = []; currentPage = page; queueVisible = false
    }
    function goBack() {
        if (backStack.length === 0) return
        const back = backStack.slice(); const page = back.pop()
        const forward = forwardStack.slice(); forward.push(currentPage)
        backStack = back; forwardStack = forward; currentPage = page; queueVisible = false
    }
    function goForward() {
        if (forwardStack.length === 0) return
        const forward = forwardStack.slice(); const page = forward.pop()
        const back = backStack.slice(); back.push(currentPage)
        backStack = back; forwardStack = forward; currentPage = page; queueVisible = false
    }
    function showMessages() { navigate("messages"); queueVisible = false; window.raise(); window.requestActivate() }
    Shortcut { sequence: "Esc"; enabled: window.queueVisible; onActivated: window.queueVisible = false }
    Connections {
        target: appController
        function onSessionChanged() {
            if (appController.authenticated) { window.width = 1320; window.height = 860 }
            else { window.width = 500; window.height = 740; window.currentPage = "library"; window.backStack = []; window.forwardStack = []; window.queueVisible = false }
        }
        function onRoomLeaveForCloseFinished() { window.finishClosing() }
        function onOpenMessagesRequested() { window.showMessages() }
        function onInvitationArrived(title, message) { window.toastOpensMessages = true; toastText.text = message; toast.open() }
        function onInfoMessage(message) { window.toastOpensMessages = false; toastText.text = message; toast.open() }
        function onSongSaved() { window.navigate("library") }
        function onSongDeleted() { window.navigate("library") }
        function onErrorMessageChanged() {
            if (appController.errorMessage.length > 0) {
                toastText.text = appController.errorMessage
                window.toastOpensMessages = false
                toast.open()
            }
        }
    }
    Popup {
        id: toast
        x: window.width - width - 22; y: 72; width: 310; height: 96
        padding: 18; closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: Theme.surface; radius: Theme.radius; border.color: Theme.border }
        Column { spacing: 8
            Text { text: "Futari Music"; color: Theme.accent; font.bold: true }
            Text { id: toastText; color: Theme.text; width: 275; wrapMode: Text.Wrap }
        }
        Timer { interval: 7000; running: toast.visible; onTriggered: toast.close() }
        MouseArea { anchors.fill: parent; enabled: window.toastOpensMessages; onClicked: { toast.close(); window.showMessages() } }
    }
    Rectangle {
        id: surfaceMask
        anchors.fill: parent
        radius: window.visibility === Window.Maximized ? 0 : 13
        color: "white"
        antialiasing: true
        visible: false
        layer.enabled: true
    }
    Item {
        anchors.fill: parent
        layer.enabled: window.visibility !== Window.Maximized
        // 0.5 阈值配合 1.0 spread 保留透明区域与覆盖率渐变，避免二值窗口掩码锯齿。
        layer.effect: MultiEffect { maskEnabled: true; maskSource: surfaceMask; maskThresholdMin: 0.5; maskSpreadAtMin: 1 }
    Rectangle {
        anchors.fill: parent
        radius: window.visibility === Window.Maximized ? 0 : 13
        color: appController.authenticated ? Theme.background : Theme.authBackground
        border.width: window.visibility === Window.Maximized ? 0 : 1
        border.color: appController.authenticated ? Theme.border : Theme.authBorder
        antialiasing: true
        clip: true
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: appController.authenticated ? Theme.background : Theme.authBackground
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: { window.queueVisible = false; window.startSystemMove() }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 8
                RowLayout {
                    spacing: 8
                    SvgIcon { name: "music-note"; color: Theme.iconAccent; iconSize: 20 }
                    Text { text: "Futari Music"; color: appController.authenticated ? Theme.text : Theme.authText; font.pixelSize: 13; font.weight: Font.Medium }
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: appController.authenticated && !!appController.roomState.room
                    text: appController.roomState.room ? appController.roomState.room.name : ""
                    color: Theme.muted; font.pixelSize: 12; elide: Text.ElideRight
                    Layout.maximumWidth: 180
                }
                FutariToolButton {
                    iconName: "window-minimize"; iconColor: Theme.iconPrimary; iconSize: 21
                    implicitWidth: 38; implicitHeight: 38; onClicked: window.showMinimized()
                    ToolTip.visible: hovered; ToolTip.text: "最小化"
                }
                FutariToolButton {
                    iconName: window.visibility === Window.Maximized ? "window-restore" : "window-maximize"
                    iconColor: Theme.iconPrimary; iconSize: 21
                    implicitWidth: 38; implicitHeight: 38
                    onClicked: window.visibility === Window.Maximized ? window.showNormal() : window.showMaximized()
                    ToolTip.visible: hovered; ToolTip.text: window.visibility === Window.Maximized ? "还原" : "最大化"
                }
                FutariToolButton {
                    iconName: "window-close"; iconColor: Theme.iconPrimary; iconSize: 21
                    hoverColor: Theme.danger; implicitWidth: 38; implicitHeight: 38; onClicked: window.close()
                    ToolTip.visible: hovered; ToolTip.text: "关闭"
                }
            }
        }
        Item {
            id: pageArea
            Layout.fillWidth: true; Layout.fillHeight: true
            LoginPage { anchors.fill: parent; visible: !appController.authenticated }
            RowLayout {
                anchors.fill: parent; spacing: 0; visible: appController.authenticated
                Sidebar { Layout.fillHeight: true; currentPage: window.currentPage === "songEdit" ? "library" : window.currentPage; onNavigate: page => window.navigate(page) }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 72; color: Theme.background
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 26; anchors.rightMargin: 26; spacing: 10
                            FutariToolButton {
                                iconName: "arrow-left"; enabled: window.backStack.length > 0
                                iconColor: enabled ? Theme.iconPrimary : Theme.iconDisabled
                                onClicked: window.goBack(); ToolTip.visible: hovered; ToolTip.text: "后退"
                            }
                            FutariToolButton {
                                iconName: "arrow-right"; enabled: window.forwardStack.length > 0
                                iconColor: enabled ? Theme.iconPrimary : Theme.iconDisabled
                                onClicked: window.goForward(); ToolTip.visible: hovered; ToolTip.text: "前进"
                            }
                            Text { text: "听见彼此"; color: Theme.text; font.pixelSize: 20; font.bold: true }
                            Item { Layout.fillWidth: true }
                            FutariToolButton {
                                iconName: Theme.dark ? "sun" : "moon"; iconColor: Theme.iconPrimary
                                onClicked: appController.darkMode = !appController.darkMode
                                ToolTip.visible: hovered; ToolTip.text: "切换浅色/深色主题"
                            }
                            FutariToolButton {
                                iconName: "account"; iconColor: Theme.iconPrimary
                                onClicked: window.navigate("account")
                                ToolTip.visible: hovered; ToolTip.text: "账户"
                            }
                            FutariToolButton {
                                iconName: "message"; iconColor: Theme.iconPrimary; onClicked: window.showMessages()
                                ToolTip.visible: hovered; ToolTip.text: "消息与邀请"
                                Rectangle {
                                    visible: appController.unreadInvites > 0
                                    anchors.right: parent.right; anchors.top: parent.top
                                    width: 9; height: 9; radius: 5; color: Theme.danger
                                    border.color: Theme.background; border.width: 1
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
                        StackLayout {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            currentIndex: ({library:0, rooms:1, partners:2, playlists:3, serverPlaylists:3, messages:4, lyrics:5, admin:6, account:7, songEdit:8, transfers:9, settings:10, tools:11})[window.currentPage] ?? 0
                            LibraryPage {
                                onTransfersRequested: window.navigate("transfers")
                                onEditSongRequested: song => {
                                    window.editingSong = song
                                    window.navigate("songEdit")
                                    appController.refreshAlbums()
                                }
                            }
                            RoomPage {}
                            PartnerPage {}
                            PlaylistPage { serverMode: window.currentPage === "serverPlaylists" }
                            MessagesPage {}
                            LyricsPage {}
                            AdminPage {}
                            AccountPage {}
                            SongEditPage { song: window.editingSong; onBackRequested: window.navigate("library") }
                            TransferPage {}
                            SettingsPage {}
                            ManagementToolsPage {}
                        }
                    }
                    PlayerBar {
                        Layout.fillWidth: true
                        onOpenQueue: window.queueVisible = !window.queueVisible
                        onOpenLyrics: window.navigate("lyrics")
                    }
                }
            }
            MouseArea {
                anchors.fill: parent
                z: 1
                visible: window.queueVisible
                onClicked: window.queueVisible = false
            }
            QueueDrawer {
                anchors.top: parent.top
                anchors.topMargin: 72
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 100
                z: 2
                visible: window.queueVisible
            }
        }
    }
    }
    MouseArea {
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 5; z: 10; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 5; z: 10; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeHorCursor
        onPressed: window.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 5; z: 10; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeVerCursor
        onPressed: window.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 5; z: 10; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeVerCursor
        onPressed: window.startSystemResize(Qt.BottomEdge)
    }
    MouseArea {
        anchors.left: parent.left; anchors.top: parent.top
        width: 12; height: 12; z: 11; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }
    MouseArea {
        anchors.right: parent.right; anchors.top: parent.top
        width: 12; height: 12; z: 11; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }
    MouseArea {
        anchors.left: parent.left; anchors.bottom: parent.bottom
        width: 12; height: 12; z: 11; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeBDiagCursor
        onPressed: window.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }
    MouseArea {
        anchors.right: parent.right; anchors.bottom: parent.bottom
        width: 12; height: 12; z: 11; enabled: window.visibility !== Window.Maximized
        cursorShape: Qt.SizeFDiagCursor
        onPressed: window.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
    Rectangle {
        anchors.fill: parent
        z: 100
        radius: window.visibility === Window.Maximized ? 0 : 13
        color: "transparent"
        border.width: window.visibility === Window.Maximized ? 0 : 1
        border.color: appController.authenticated ? Theme.border : Theme.authBorder
        antialiasing: true
    }
}
