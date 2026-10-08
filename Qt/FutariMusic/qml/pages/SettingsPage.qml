import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

Item {
    id: root
    function formatBytes(bytes) {
        const value = Math.max(0, Number(bytes || 0))
        if (value < 1024) return value + " B"
        if (value < 1024 * 1024) return (value / 1024).toFixed(1) + " KB"
        if (value < 1024 * 1024 * 1024) return (value / (1024 * 1024)).toFixed(1) + " MB"
        return (value / (1024 * 1024 * 1024)).toFixed(2) + " GB"
    }
    FolderDialog { id: downloadDirectoryPicker; title: "选择下载目录"; onAccepted: appController.downloadDirectory = selectedFolder.toString() }
    FolderDialog { id: cacheDirectoryPicker; title: "选择歌曲缓存目录"; onAccepted: appController.cacheDirectory = selectedFolder.toString() }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 0
        Text { text: "设置"; color: Theme.text; font.pixelSize: 30; font.bold: true; Layout.fillHeight: false; Layout.bottomMargin: 18 }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: false; Layout.minimumHeight: 44; Layout.preferredHeight: 44; Layout.maximumHeight: 44; spacing: 22
            Item {
                implicitWidth: sectionLabel.implicitWidth + 10; Layout.fillHeight: true
                Text { id: sectionLabel; anchors.centerIn: parent; text: "下载与缓存"; color: Theme.accent; font.pixelSize: 14; font.weight: Font.DemiBold }
                Rectangle { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; width: sectionLabel.width; height: 3; radius: 2; color: Theme.accent }
            }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: false; Layout.minimumHeight: 1; Layout.preferredHeight: 1; Layout.maximumHeight: 1; color: Theme.border }
        Flickable {
            id: content; Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0; clip: true
            contentWidth: width; contentHeight: settingsColumn.implicitHeight + 40
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: settingsColumn; x: 10; y: 20; width: content.width - 20; height: implicitHeight; spacing: 0
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
            ScrollBar.vertical: ScrollBar {}
        }
    }
    FutariDialog {
        id: clearCacheDialog; title: "清理歌曲缓存？"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        Label { text: "将删除约 " + root.formatBytes(appController.cacheUsageBytes) + " 的未使用歌曲缓存。已下载歌曲不会受影响。"; color: Theme.text; wrapMode: Text.Wrap; width: 360 }
        onAccepted: appController.clearSongCache()
    }
}
