import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

Item {
    id: root
    property int activeTab: 0
    function formatBytes(bytes) {
        const value = Math.max(0, Number(bytes || 0))
        if (value < 1024) return value + " B"
        if (value < 1024 * 1024) return (value / 1024).toFixed(1) + " KB"
        if (value < 1024 * 1024 * 1024) return (value / (1024 * 1024)).toFixed(1) + " MB"
        return (value / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
    FolderDialog { id: downloadDirectoryPicker; title: "选择下载目录"; onAccepted: appController.downloadDirectory = selectedFolder.toString() }
    FolderDialog { id: cacheDirectoryPicker; title: "选择歌曲缓存目录"; onAccepted: appController.cacheDirectory = selectedFolder.toString() }
    ButtonGroup { id: lyricDisplayModeGroup; exclusive: true }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 0
        Text { text: "设置"; color: Theme.text; font.pixelSize: 30; font.bold: true; Layout.fillHeight: false; Layout.bottomMargin: 18 }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: false; Layout.minimumHeight: 44; Layout.preferredHeight: 44; Layout.maximumHeight: 44; spacing: 8
            TabBar {
                id: settingsTabs
                Layout.fillHeight: true
                currentIndex: root.activeTab
                onCurrentIndexChanged: root.activeTab = currentIndex
                background: Item {}
                TabButton {
                    id: generalTab
                    text: "常规设置"
                    background: Rectangle {
                        color: generalTab.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        Rectangle { visible: generalTab.checked; width: generalTab.contentItem.implicitWidth; height: 3; radius: 2; color: Theme.accent; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    contentItem: Text { text: generalTab.text; color: generalTab.checked ? Theme.accent : Theme.muted; font.pixelSize: 14; font.weight: generalTab.checked ? Font.DemiBold : Font.Normal; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    id: downloadsTab
                    text: "下载与缓存"
                    background: Rectangle {
                        color: downloadsTab.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        Rectangle { visible: downloadsTab.checked; width: downloadsTab.contentItem.implicitWidth; height: 3; radius: 2; color: Theme.accent; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    contentItem: Text { text: downloadsTab.text; color: downloadsTab.checked ? Theme.accent : Theme.muted; font.pixelSize: 14; font.weight: downloadsTab.checked ? Font.DemiBold : Font.Normal; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    id: lyricsTab
                    text: "歌词设置"
                    background: Rectangle {
                        color: lyricsTab.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        Rectangle { visible: lyricsTab.checked; width: lyricsTab.contentItem.implicitWidth; height: 3; radius: 2; color: Theme.accent; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    contentItem: Text { text: lyricsTab.text; color: lyricsTab.checked ? Theme.accent : Theme.muted; font.pixelSize: 14; font.weight: lyricsTab.checked ? Font.DemiBold : Font.Normal; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    id: updatesTab
                    text: "更新与升级"
                    background: Rectangle {
                        color: updatesTab.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        Rectangle { visible: updatesTab.checked; width: updatesTab.contentItem.implicitWidth; height: 3; radius: 2; color: Theme.accent; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    contentItem: Text { text: updatesTab.text; color: updatesTab.checked ? Theme.accent : Theme.muted; font.pixelSize: 14; font.weight: updatesTab.checked ? Font.DemiBold : Font.Normal; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: false; Layout.minimumHeight: 1; Layout.preferredHeight: 1; Layout.maximumHeight: 1; color: Theme.border }
        Flickable {
            id: content; Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0; clip: true
            contentWidth: width
            contentHeight: (root.activeTab === 0
                ? generalColumn.implicitHeight
                : root.activeTab === 1 ? settingsColumn.implicitHeight
                : root.activeTab === 2 ? lyricsSettingsColumn.implicitHeight
                : updateSettingsColumn.implicitHeight) + 40
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: generalColumn
                x: 10; y: 20; width: content.width - 20; height: implicitHeight; spacing: 0
                visible: root.activeTab === 0
                Text {
                    text: "常规设置"
                    color: Theme.text
                    font.pixelSize: 19
                    font.bold: true
                    Layout.bottomMargin: 8
                }
                Text {
                    text: "设置关闭主面板时客户端的行为。"
                    color: Theme.muted
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.bottomMargin: 16
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                    Layout.bottomMargin: 18
                }
                Text {
                    text: "关闭主面板"
                    color: Theme.muted
                    font.pixelSize: 14
                    Layout.bottomMargin: 8
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                        RadioButton {
                            id: minimizeToTrayOption
                            text: "最小化到托盘，不退出程序"
                            checked: trayAvailable && appController.minimizeToTrayOnClose
                            enabled: trayAvailable
                            Layout.preferredHeight: 40
                            Layout.fillWidth: true
                            onClicked: appController.minimizeToTrayOnClose = true
                            background: Rectangle {
                                color: minimizeToTrayOption.hovered ? Theme.hover : "transparent"
                                radius: Theme.radiusSmall
                            }
                            indicator: Rectangle {
                                x: 10
                                y: (minimizeToTrayOption.height - height) / 2
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 10
                                color: "transparent"
                                border.width: minimizeToTrayOption.activeFocus ? 3 : 2
                                border.color: minimizeToTrayOption.checked || minimizeToTrayOption.hovered
                                    ? Theme.accent : Theme.border
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: Theme.accent
                                    visible: minimizeToTrayOption.checked
                                }
                            }
                            contentItem: Text {
                                text: minimizeToTrayOption.text
                                color: minimizeToTrayOption.enabled ? Theme.text : Theme.muted
                                font.pixelSize: 14
                                leftPadding: minimizeToTrayOption.indicator.width + 20
                                rightPadding: 10
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Text {
                            visible: !trayAvailable
                            text: "当前系统未检测到托盘，关闭窗口会退出客户端。"
                            color: Theme.muted
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                        RadioButton {
                            id: exitOnCloseOption
                            text: "退出程序"
                            checked: !trayAvailable || !appController.minimizeToTrayOnClose
                            Layout.preferredHeight: 40
                            Layout.fillWidth: true
                            onClicked: appController.minimizeToTrayOnClose = false
                            background: Rectangle {
                                color: exitOnCloseOption.hovered ? Theme.hover : "transparent"
                                radius: Theme.radiusSmall
                            }
                            indicator: Rectangle {
                                x: 10
                                y: (exitOnCloseOption.height - height) / 2
                                implicitWidth: 20
                                implicitHeight: 20
                                radius: 10
                                color: "transparent"
                                border.width: exitOnCloseOption.activeFocus ? 3 : 2
                                border.color: exitOnCloseOption.checked || exitOnCloseOption.hovered
                                    ? Theme.accent : Theme.border
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: Theme.accent
                                    visible: exitOnCloseOption.checked
                                }
                            }
                            contentItem: Text {
                                text: exitOnCloseOption.text
                                color: Theme.text
                                font.pixelSize: 14
                                leftPadding: exitOnCloseOption.indicator.width + 20
                                rightPadding: 10
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                    Layout.topMargin: 20
                    Layout.bottomMargin: 16
                }
                Text { text: "播放列表"; color: Theme.muted; font.pixelSize: 14; Layout.bottomMargin: 6 }
                FutariCheckBox {
                    text: "退出应用后保存播放列表"
                    checked: appController.savePlaybackQueue
                    onClicked: appController.savePlaybackQueue = checked
                    Layout.fillWidth: true
                }
                Text {
                    text: "重新启动并登录后，恢复本地播放队列。"
                    color: Theme.muted
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 30
                }
            }
            ColumnLayout {
                id: settingsColumn; x: 10; y: 20; width: content.width - 20; height: implicitHeight; spacing: 0
                visible: root.activeTab === 1
                Text { text: "下载与缓存"; color: Theme.text; font.pixelSize: 19; font.bold: true; Layout.bottomMargin: 8 }
                Text { text: "下载文件与播放缓存保存在不同目录，清理缓存不会删除已下载的歌曲。"; color: Theme.muted; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.bottomMargin: 16 }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.bottomMargin: 18 }
                Text { text: "下载目录"; color: Theme.text; font.pixelSize: 15; font.weight: Font.Medium }
                Text { text: "新下载任务将使用此位置；已开始的任务继续保存到创建任务时确定的目录。"; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.topMargin: 5; Layout.bottomMargin: 10 }
                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    FutariTextField { text: appController.downloadDirectory; readOnly: true; Layout.fillWidth: true }
                    FutariButton { text: "更改目录"; onClicked: downloadDirectoryPicker.open() }
                    FutariButton { text: "打开文件夹"; onClicked: appController.openLocalFolder(appController.downloadDirectory) }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 20; Layout.bottomMargin: 18 }
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "开启歌曲缓存"; color: Theme.text; font.pixelSize: 15; font.weight: Font.Medium }
                        Text { text: "关闭后停止生成新的持久缓存，不影响播放，也不会删除已有缓存。"; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Switch {
                        checked: appController.cacheEnabled
                        onToggled: appController.cacheEnabled = checked
                        indicator: Rectangle {
                            implicitWidth: 46; implicitHeight: 26; x: parent.width - width; y: parent.height / 2 - height / 2; radius: 13
                            color: parent.checked ? Theme.accent : Theme.secondary; border.color: parent.checked ? Theme.accent : Theme.border
                            Rectangle { x: parent.parent.checked ? parent.width - width - 3 : 3; y: 3; width: 20; height: 20; radius: 10; color: "white"; Behavior on x { NumberAnimation { duration: Theme.motionFast } } }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: 18; Layout.bottomMargin: 18 }
                Text { text: "歌曲缓存"; color: Theme.text; font.pixelSize: 15; font.weight: Font.Medium }
                Text { text: "缓存状态：" + (appController.cacheEnabled ? "已开启" : "已关闭") + "   ·   当前占用：" + root.formatBytes(appController.cacheUsageBytes); color: Theme.muted; font.pixelSize: 13; Layout.topMargin: 8; Layout.bottomMargin: 10 }
                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    FutariTextField { text: appController.cacheDirectory; readOnly: true; Layout.fillWidth: true }
                    FutariButton { text: "更改目录"; onClicked: cacheDirectoryPicker.open() }
                    FutariButton { text: "打开文件夹"; onClicked: appController.openLocalFolder(appController.cacheDirectory) }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.topMargin: 14
                    Text { text: "清理只会移除应用可识别的歌曲缓存，并跳过当前正在播放或写入的文件。"; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    FutariButton { text: "清理缓存"; iconName: "trash"; onClicked: clearCacheDialog.open() }
                }
            }
            ColumnLayout {
                id: lyricsSettingsColumn
                x: 10; y: 20; width: content.width - 20; height: implicitHeight; spacing: 0
                visible: root.activeTab === 2
                Text { text: "歌词设置"; color: Theme.text; font.pixelSize: 19; font.bold: true; Layout.bottomMargin: 8 }
                Text {
                    text: "选择逐字歌词的显示方式。自动模式会优先显示逐字卡拉 OK 歌词，没有逐字歌词时回退到逐行歌词。"
                    color: Theme.muted; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.bottomMargin: 16
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.bottomMargin: 18 }
                Text { text: "歌词显示方式"; color: Theme.text; font.pixelSize: 15; font.weight: Font.Medium; Layout.bottomMargin: 8 }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    RadioButton {
                        id: automaticLyricsOption
                        text: "自动卡拉 OK"
                        ButtonGroup.group: lyricDisplayModeGroup
                        checked: appController.lyricsDisplayMode === "automatic"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onClicked: appController.lyricsDisplayMode = "automatic"
                        background: Rectangle { color: automaticLyricsOption.hovered ? Theme.hover : "transparent"; radius: Theme.radiusSmall }
                        indicator: Rectangle {
                            x: 10; y: (automaticLyricsOption.height - height) / 2
                            implicitWidth: 20; implicitHeight: 20; radius: 10
                            color: "transparent"
                            border.width: automaticLyricsOption.activeFocus ? 3 : 2
                            border.color: automaticLyricsOption.checked || automaticLyricsOption.hovered ? Theme.accent : Theme.border
                            Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5; color: Theme.accent; visible: automaticLyricsOption.checked }
                        }
                        contentItem: Text {
                            text: automaticLyricsOption.text; color: Theme.text; font.pixelSize: 14
                            leftPadding: automaticLyricsOption.indicator.width + 20; rightPadding: 10; verticalAlignment: Text.AlignVCenter
                        }
                    }
                    RadioButton {
                        id: lineLyricsOption
                        text: "仅逐行歌词"
                        ButtonGroup.group: lyricDisplayModeGroup
                        checked: appController.lyricsDisplayMode === "line"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onClicked: appController.lyricsDisplayMode = "line"
                        background: Rectangle { color: lineLyricsOption.hovered ? Theme.hover : "transparent"; radius: Theme.radiusSmall }
                        indicator: Rectangle {
                            x: 10; y: (lineLyricsOption.height - height) / 2
                            implicitWidth: 20; implicitHeight: 20; radius: 10
                            color: "transparent"
                            border.width: lineLyricsOption.activeFocus ? 3 : 2
                            border.color: lineLyricsOption.checked || lineLyricsOption.hovered ? Theme.accent : Theme.border
                            Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5; color: Theme.accent; visible: lineLyricsOption.checked }
                        }
                        contentItem: Text {
                            text: lineLyricsOption.text; color: Theme.text; font.pixelSize: 14
                            leftPadding: lineLyricsOption.indicator.width + 20; rightPadding: 10; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
            ColumnLayout {
                id: updateSettingsColumn
                x: 10; y: 20; width: content.width - 20; height: implicitHeight; spacing: 0
                visible: root.activeTab === 3
                Text { text: "更新与升级"; color: Theme.text; font.pixelSize: 19; font.bold: true; Layout.bottomMargin: 8 }
                Text { text: "检查当前服务器对应的兼容客户端版本，并按本机系统下载安装包。"; color: Theme.muted; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true; Layout.bottomMargin: 16 }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.bottomMargin: 18 }
                RowLayout {
                    Layout.fillWidth: true; Layout.bottomMargin: 8
                    Text { text: "服务端版本"; color: Theme.muted; font.pixelSize: 13; Layout.preferredWidth: 150 }
                    Text { text: updateManager.serverVersion || "尚未检查"; color: Theme.text; font.pixelSize: 14 }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.bottomMargin: 8
                    Text { text: "当前客户端"; color: Theme.muted; font.pixelSize: 13; Layout.preferredWidth: 150 }
                    Text { text: updateManager.currentReleaseTag; color: Theme.text; font.pixelSize: 14 }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.bottomMargin: 16
                    Text { text: "兼容客户端"; color: Theme.muted; font.pixelSize: 13; Layout.preferredWidth: 150 }
                    Text { text: updateManager.releaseTag || "暂无适用于此平台的发行包"; color: Theme.text; font.pixelSize: 14; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.bottomMargin: 18 }
                RowLayout {
                    Layout.fillWidth: true; spacing: 12; Layout.bottomMargin: 8
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: "自动静默更新到兼容版本"; color: Theme.text; font.pixelSize: 15; font.weight: Font.Medium }
                        Text { text: "启动时发现更新后自动下载并安装。Ubuntu 安装 .deb 时仍会请求系统授权。"; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Switch {
                        checked: updateManager.autoSilentUpdate
                        onToggled: updateManager.autoSilentUpdate = checked
                        indicator: Rectangle {
                            implicitWidth: 46; implicitHeight: 26; x: parent.width - width; y: parent.height / 2 - height / 2; radius: 13
                            color: parent.checked ? Theme.accent : Theme.secondary; border.color: parent.checked ? Theme.accent : Theme.border
                            Rectangle { x: parent.parent.checked ? parent.width - width - 3 : 3; y: 3; width: 20; height: 20; radius: 10; color: "white"; Behavior on x { NumberAnimation { duration: Theme.motionFast } } }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.topMargin: 12
                    Text { text: updateManager.statusMessage; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    FutariButton {
                        text: updateManager.checking ? "检查中…" : "检查更新"
                        prominent: true
                        enabled: !updateManager.checking && !updateManager.downloading && !updateManager.installing
                        onClicked: updateManager.checkForUpdates(false)
                    }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
    FutariDialog {
        id: updateDialog
        title: "发现兼容客户端更新"
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(500, parent.width - 40)
        standardButtons: Dialog.NoButton
        closePolicy: updateManager.downloading || updateManager.installing
            ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)
        contentItem: ColumnLayout {
            spacing: 16
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 84; radius: Theme.radius; color: Theme.secondary
                RowLayout {
                    anchors.fill: parent; anchors.margins: 14; spacing: 14
                    Rectangle {
                        Layout.preferredWidth: 48; Layout.preferredHeight: 48; radius: 14; color: Theme.surface
                        SvgIcon { anchors.centerIn: parent; name: "download"; iconSize: 22; color: Theme.iconAccent }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 5
                        Text { text: "兼容客户端版本"; color: Theme.muted; font.pixelSize: 12 }
                        Text {
                            text: updateManager.releaseTag
                            color: Theme.text
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text { text: "服务端 " + (updateManager.serverVersion || "未知"); color: Theme.muted; font.pixelSize: 12 }
                    }
                }
            }
            RowLayout {
                visible: updateManager.downloading || updateManager.installing
                Layout.fillWidth: true
                Text { text: updateManager.installing ? "正在安装" : "下载进度"; color: Theme.text; font.pixelSize: 13; font.weight: Font.Medium }
                Item { Layout.fillWidth: true }
                Text {
                    text: updateManager.progressIndeterminate && updateManager.downloading
                        ? "正在连接…" : updateManager.downloadProgress + "%"
                    color: Theme.muted; font.pixelSize: 12
                }
            }
            ProgressBar {
                id: updateProgressBar
                Layout.fillWidth: true
                visible: updateManager.downloading || updateManager.installing
                from: 0; to: 100; value: updateManager.downloadProgress
                indeterminate: updateManager.progressIndeterminate && updateManager.downloading
                onIndeterminateChanged: if (!indeterminate) progressSegment.x = 0
                background: Rectangle { implicitHeight: 6; radius: 3; color: Theme.secondary }
                contentItem: Item {
                    id: progressTrack
                    clip: true
                    Rectangle {
                        id: progressSegment
                        width: updateProgressBar.indeterminate
                            ? Math.max(28, progressTrack.width * 0.28)
                            : progressTrack.width * Math.max(0, Math.min(1, updateManager.downloadProgress / 100))
                        height: parent.height; radius: 3; color: Theme.accent
                        SequentialAnimation on x {
                            running: updateProgressBar.indeterminate
                            loops: Animation.Infinite
                            NumberAnimation {
                                from: -progressSegment.width
                                to: progressTrack.width
                                duration: 900
                                easing.type: Easing.InOutQuad
                            }
                        }
                    }
                }
            }
            Text {
                text: updateManager.statusMessage || "下载完成后将校验安装包，再启动安装程序。"
                color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
        }
        footer: Item {
            implicitHeight: footerButtons.implicitHeight + 14
            RowLayout {
                id: footerButtons
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 22
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                spacing: 10
                Item { Layout.fillWidth: true }
                FutariButton {
                    visible: !updateManager.installing
                    text: updateManager.downloading ? "取消下载" : "稍后"
                    onClicked: {
                        if (updateManager.downloading) updateManager.cancelDownload()
                        else updateDialog.close()
                    }
                }
                FutariButton {
                    visible: !updateManager.downloading && !updateManager.installing
                    text: "下载并安装"
                    prominent: true
                    onClicked: updateManager.startUpdate()
                }
            }
        }
    }
    Connections {
        target: updateManager
        function onUpdatePromptRequested() { updateDialog.open() }
        function onUpdateSucceeded() { updateDialog.close() }
    }
    FutariDialog {
        id: clearCacheDialog; title: "清理歌曲缓存？"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: "将删除约 " + root.formatBytes(appController.cacheUsageBytes) + " 的未使用歌曲缓存。已下载歌曲不会受影响。"; color: Theme.text; wrapMode: Text.Wrap; width: 360 }
        onAccepted: appController.clearSongCache()
    }
}
