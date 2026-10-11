import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

FutariDialog {
    id: root
    objectName: "lyricsCompletionDialog"
    title: "补充歌曲歌词"
    anchors.centerIn: parent
    width: Math.min(parent.width - 24, 1040)
    height: Math.min(parent.height - 24, 760)
    property var controller: appController.lyricsCompletion
    property int activeTab: 0
    property url karaokeLyricsDirectory: ""
    property url lineLyricsDirectory: ""
    readonly property string activeType: activeTab === 0 ? "karaoke" : "line"
    readonly property url activeDirectory: activeTab === 0 ? karaokeLyricsDirectory : lineLyricsDirectory

    function scanCurrentType() {
        controller.scanServerSongs(activeType)
        if (activeDirectory.toString().length > 0)
            controller.scanDirectory(activeDirectory)
    }

    onOpened: {
        // 关闭弹窗时上传仍可在后台完成；重新打开继续显示这次操作的真实进度。
        if (!controller.busy) {
            karaokeLyricsDirectory = ""
            lineLyricsDirectory = ""
            activeTab = 0
            controller.scanServerSongs("karaoke")
        }
    }

    Connections {
        target: appController
        function onSessionChanged() {
            if (!appController.admin) root.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        TabBar {
            id: lyricsTabs
            Layout.fillWidth: true
            enabled: !root.controller.busy
            currentIndex: root.activeTab
            onCurrentIndexChanged: {
                const tabChanged = root.activeTab !== currentIndex
                root.activeTab = currentIndex
                if (tabChanged) root.scanCurrentType()
            }
            background: Item {}

            Repeater {
                model: ["补充逐字歌词", "补充逐行歌词"]
                delegate: TabButton {
                    required property int index
                    required property string modelData
                    text: modelData
                    background: Rectangle {
                        color: parent.hovered ? Theme.hover : "transparent"
                        radius: Theme.radiusSmall
                        Rectangle {
                            visible: parent.parent.checked
                            width: parent.parent.contentItem.implicitWidth
                            height: 3
                            radius: 2
                            color: Theme.accent
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    contentItem: Text {
                        text: modelData
                        color: parent.checked ? Theme.accent : Theme.muted
                        font.pixelSize: 14
                        font.weight: parent.checked ? Font.DemiBold : Font.Normal
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        RowLayout {
            Layout.fillWidth: true
            FutariButton {
                text: "选择歌词目录"
                iconName: "plus"
                enabled: !root.controller.busy && appController.admin
                onClicked: folderDialog.open()
            }
            FutariButton {
                text: "重新扫描服务器"
                enabled: !root.controller.busy && appController.admin
                onClicked: root.scanCurrentType()
            }
            Text {
                Layout.fillWidth: true
                text: root.activeDirectory.toString().length > 0
                      ? decodeURIComponent(root.activeDirectory.toString().split("/").slice(-2).join("/"))
                      : "选择目录后将递归扫描其中的 LRC/TXT 文件"
                color: Theme.muted
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                text: "已选文件 " + root.controller.selectedCount + " / " + root.controller.candidateCount + " 首"
                color: Theme.muted
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.activeTab === 0
                  ? "只匹配包含逐字时间标签的 LRC/TXT；文件名按“歌曲-歌手”或“歌手-歌曲”匹配。"
                  : "只匹配逐行歌词 LRC/TXT；文件名按“歌曲-歌手”或“歌手-歌曲”匹配。"
            color: Theme.muted
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            text: root.controller.summary
            color: Theme.text
            wrapMode: Text.Wrap
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: root.controller.busy || root.controller.pendingCount > 0
            indeterminate: root.controller.busy && root.controller.pendingCount === 0
            from: 0
            to: Math.max(1, root.controller.pendingCount)
            value: root.controller.completed
        }

        ListView {
            id: results
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: root.controller.rows
            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: lyricsRow
                required property int index
                required property var modelData
                width: results.width
                height: 112
                radius: Theme.radius
                color: Theme.secondary

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 3
                        Text {
                            text: lyricsRow.modelData.title
                            color: Theme.text
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: lyricsRow.modelData.artist || "未知歌手"
                            color: Theme.muted
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: lyricsRow.modelData.status
                            color: Theme.muted
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: lyricsRow.modelData.preview || "选择匹配文件后显示歌词预览"
                            color: Theme.muted
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            font.pixelSize: 12
                        }
                    }

                    FutariComboBox {
                        objectName: "lyricsFileChoice" + lyricsRow.index
                        Layout.preferredWidth: Math.min(360, root.width * 0.38)
                        model: root.controller.fileNames
                        currentIndex: lyricsRow.modelData.fileIndex
                        enabled: !root.controller.busy && !lyricsRow.modelData.done
                        Accessible.name: "选择" + lyricsRow.modelData.title + "的歌词文件"
                        onActivated: fileIndex => root.controller.selectFile(lyricsRow.index, fileIndex)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: root.controller.pendingCount > 0
                      ? "处理进度 " + root.controller.completed + " / " + root.controller.pendingCount
                      : ""
                color: Theme.muted
            }
            Item { Layout.fillWidth: true }
            FutariButton { text: "关闭"; enabled: !root.controller.busy; onClicked: root.close() }
            FutariButton {
                text: "确认补全（" + root.controller.selectedCount + "）"
                prominent: true
                enabled: !root.controller.busy && root.controller.selectedCount > 0 && appController.admin
                onClicked: root.controller.submit()
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "选择歌词文件所在目录"
        onAccepted: {
            if (root.activeTab === 0)
                root.karaokeLyricsDirectory = selectedFolder
            else
                root.lineLyricsDirectory = selectedFolder
            root.controller.scanDirectory(selectedFolder)
        }
    }
}
