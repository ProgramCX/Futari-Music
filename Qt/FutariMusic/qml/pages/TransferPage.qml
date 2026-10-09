import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    id: root
    // 标签页是展示状态；任务状态/进度来自 TransferManager，页面只做筛选。
    property int currentTab: 0
    readonly property var tabNames: ["正在上传", "正在下载", "上传的歌曲", "下载的歌曲"]

    function formatBytes(bytes) {
        const value = Math.max(0, Number(bytes || 0))
        if (value < 1024) return value + " B"
        if (value < 1024 * 1024) return (value / 1024).toFixed(1) + " KB"
        if (value < 1024 * 1024 * 1024) return (value / (1024 * 1024)).toFixed(1) + " MB"
        return (value / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
    function formatSpeed(bytes) { return Number(bytes || 0) > 0 ? formatBytes(bytes) + "/s" : "速度计算中" }
    function statusText(status) {
        return ({waiting:"等待中", uploading:"上传中", downloading:"下载中", paused:"已暂停",
                 success:"已完成", failed:"失败", cancelled:"已取消", interrupted:"中断，可继续"})[status] || status
    }
    function currentRows() {
        if (currentTab === 2) return appController.uploadedSongs
        const tasks = appController.transferTasks
        if (currentTab === 3) return tasks.filter(task => task.kind === "download" && task.status === "success")
        const kind = currentTab === 0 ? "upload" : "download"
        return tasks.filter(task => task.kind === kind && task.status !== "success" && task.status !== "cancelled")
    }
    function batchAction(action) {
        const kind = currentTab === 0 ? "upload" : "download"
        if (action === "pause") appController.transferManager.pauseAll(kind)
        else if (action === "resume") appController.transferManager.resumeAll(kind)
        else if (action === "cancel") appController.transferManager.cancelAll(kind)
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 3
                Text { text: "上传与下载"; color: Theme.text; font.pixelSize: 30; font.bold: true }
                Text { text: "查看传输进度和已完成记录"; color: Theme.muted; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            FutariButton { text: "全部暂停"; visible: root.currentTab < 2; onClicked: root.batchAction("pause") }
            FutariButton { text: "全部继续"; visible: root.currentTab < 2; onClicked: root.batchAction("resume") }
            FutariButton { text: "全部停止"; visible: root.currentTab < 2; onClicked: stopAllDialog.open() }
            FutariButton { text: "刷新"; visible: root.currentTab === 2; onClicked: appController.refreshUploadedSongs() }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 24
            Repeater {
                model: root.tabNames
                delegate: Item {
                    required property int index
                    required property string modelData
                    implicitWidth: tabLabel.implicitWidth + 8; implicitHeight: 42
                    Text { id: tabLabel; anchors.centerIn: parent; text: modelData; color: root.currentTab === index ? Theme.accent : Theme.muted; font.pixelSize: 14; font.weight: root.currentTab === index ? Font.DemiBold : Font.Normal }
                    Rectangle { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; width: tabLabel.width; height: 3; radius: 2; color: Theme.accent; visible: root.currentTab === index }
                    TapHandler { onTapped: root.currentTab = index }
                }
            }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        ListView {
            id: transferList
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 6
            model: root.currentRows()
            delegate: Rectangle {
                required property int index
                required property var modelData
                readonly property var itemData: modelData
                readonly property bool historyUpload: root.currentTab === 2
                readonly property bool historyDownload: root.currentTab === 3
                width: ListView.view.width; height: historyUpload ? 76 : 104; radius: Theme.radius
                color: rowHover.hovered ? Theme.hover : Theme.surface
                HoverHandler { id: rowHover }
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 12
                    Rectangle {
                        Layout.preferredWidth: 54; Layout.preferredHeight: 54; radius: 10; color: Theme.secondary; clip: true
                        SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconAccent; iconSize: 24; visible: cover.status !== Image.Ready }
                        Image { id: cover; anchors.fill: parent; source: itemData.coverUrl || ""; fillMode: Image.PreserveAspectCrop }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text {
                            Layout.fillWidth: true; text: (itemData.title || "未命名歌曲") + " · " + (itemData.artist || "未知歌手")
                            color: Theme.text; font.pixelSize: 14; font.weight: Font.Medium; elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: historyUpload
                                  ? ((itemData.album || "未关联专辑") + " · 上传于 " + new Date(itemData.createdAt || 0).toLocaleString(Qt.locale(), Locale.ShortFormat))
                                  : (historyDownload
                                     ? ((itemData.fileExists === false ? "本地文件已移除 · " : "已保存 · ") + (itemData.targetPath || ""))
                                     : ((itemData.album || "未关联专辑") + " · " + root.formatBytes(itemData.transferred) + " / " + root.formatBytes(itemData.totalBytes) + " · " + root.statusText(itemData.status)))
                            color: itemData.status === "failed" ? Theme.danger : Theme.muted; font.pixelSize: 12; elide: Text.ElideRight
                        }
                        ProgressBar {
                            visible: !historyUpload && !historyDownload && itemData.status !== "success"
                            Layout.fillWidth: true; from: 0; to: 100; value: Number(itemData.progress || 0)
                            background: Rectangle { implicitHeight: 5; radius: 3; color: Theme.secondary }
                            contentItem: Item {
                                Rectangle { width: parent.width * Math.max(0, Math.min(1, (itemData.progress || 0) / 100)); height: parent.height; radius: 3; color: itemData.status === "failed" ? Theme.danger : Theme.accent }
                            }
                        }
                        Text {
                            visible: !historyUpload && !historyDownload && itemData.message && itemData.message.length > 0
                            text: itemData.message || ""; color: Theme.muted; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true
                        }
                    }
                    Text {
                        visible: !historyUpload && !historyDownload && (itemData.status === "uploading" || itemData.status === "downloading")
                        text: root.formatSpeed(itemData.speedBytes); color: Theme.muted; font.pixelSize: 11
                    }
                    ColumnLayout {
                        visible: !historyUpload && !historyDownload
                        spacing: 4
                        FutariToolButton {
                            visible: itemData.status === "uploading" || itemData.status === "downloading" || itemData.status === "waiting"
                            iconName: "pause"; ToolTip.visible: hovered; ToolTip.text: "暂停"
                            onClicked: appController.transferManager.pauseTask(itemData.id)
                        }
                        FutariToolButton {
                            visible: itemData.status === "paused" || itemData.status === "interrupted"
                            iconName: "play"; ToolTip.visible: hovered; ToolTip.text: "继续"
                            onClicked: appController.transferManager.resumeTask(itemData.id)
                        }
                        FutariToolButton {
                            visible: itemData.status === "failed"
                            iconName: "upload"; ToolTip.visible: hovered; ToolTip.text: "重试"
                            onClicked: appController.transferManager.retryTask(itemData.id)
                        }
                    }
                    FutariToolButton {
                        visible: historyDownload && itemData.fileExists !== false
                        iconName: "play"; ToolTip.visible: hovered; ToolTip.text: "播放本地文件"
                        enabled: !appController.roomState.room
                        onClicked: appController.player.playLocalFile(itemData.targetPath || "", itemData.title || "", itemData.artist || "")
                    }
                    FutariToolButton {
                        visible: historyDownload && itemData.fileExists !== false
                        iconName: "more"; ToolTip.visible: hovered; ToolTip.text: "打开所在文件夹"
                        onClicked: appController.openLocalFolder(itemData.targetPath || "")
                    }
                    FutariToolButton {
                        visible: !historyUpload && !historyDownload && itemData.status !== "success" && itemData.status !== "cancelled"
                        iconName: "remove"; ToolTip.visible: hovered; ToolTip.text: "取消任务"
                        onClicked: appController.transferManager.cancelTask(itemData.id)
                    }
                }
            }
            ScrollBar.vertical: ScrollBar {}
            Column {
                anchors.centerIn: parent; spacing: 10
                visible: transferList.count === 0
                SvgIcon { anchors.horizontalCenter: parent.horizontalCenter; name: root.currentTab === 0 || root.currentTab === 2 ? "upload" : "download"; color: Theme.iconSecondary; iconSize: 34 }
                Text { text: ["没有上传任务", "没有下载任务", "还没有成功上传歌曲", "还没有下载歌曲"][root.currentTab]; color: Theme.muted; font.pixelSize: 14 }
            }
            footer: Item {
                width: transferList.width; height: root.currentTab === 2 && appController.hasMoreUploadedSongs ? 54 : 0
                FutariButton { anchors.centerIn: parent; visible: root.currentTab === 2 && appController.hasMoreUploadedSongs; text: "加载更多"; onClicked: appController.loadMoreUploadedSongs() }
            }
        }
    }
    FutariDialog {
        id: stopAllDialog; title: root.currentTab === 0 ? "停止全部上传任务？" : "停止全部下载任务？"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: root.currentTab === 0 ? "正在上传的任务会取消并从列表移除。" : "正在下载的任务会取消、删除临时文件并从列表移除。"; color: Theme.text; wrapMode: Text.Wrap; width: 330 }
        onAccepted: root.batchAction("cancel")
    }
    Component.onCompleted: appController.refreshUploadedSongs()
}
