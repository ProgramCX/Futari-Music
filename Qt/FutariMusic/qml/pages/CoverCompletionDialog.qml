import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FutariMusic
import "../components"

FutariDialog {
    id: root
    objectName: "coverCompletionDialog"
    title: "补全歌曲与专辑封面"
    anchors.centerIn: parent
    width: Math.min(parent.width - 24, 1040)
    height: Math.min(parent.height - 24, 730)
    property var controller: appController.coverCompletion
    property int filterIndex: 0
    Connections {
        target: appController
        function onSessionChanged() { if (!appController.admin) root.close() }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            FutariButton {
                text: "选择封面目录"; iconName: "plus"
                enabled: !root.controller.busy && appController.admin
                onClicked: folderDialog.open()
            }
            FutariComboBox {
                Layout.preferredWidth: 170
                model: ["全部结果", "歌曲封面", "专辑封面"]
                currentIndex: root.filterIndex
                onActivated: root.filterIndex = currentIndex
            }
            Item { Layout.fillWidth: true }
            Text { text: "已选 " + root.controller.selectedCount + " 项"; color: Theme.muted }
        }
        Text {
            text: "匹配顺序：歌曲-歌手 → 歌手-歌曲；仅扫描所选目录。JPG / PNG / WebP，每张 ≤ 5 MiB。专辑封面请单独选择。"
            color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true
        }
        Text { text: root.controller.summary; color: Theme.text; wrapMode: Text.Wrap; Layout.fillWidth: true }
        ProgressBar {
            Layout.fillWidth: true; visible: root.controller.busy || root.controller.total > 0
            indeterminate: root.controller.busy && root.controller.total === 0
            from: 0; to: Math.max(1, root.controller.total); value: root.controller.completed
        }
        ListView {
            id: results
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 6
            model: root.controller.rows
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: coverRow
                required property int index
                required property var modelData
                readonly property bool shown: root.filterIndex === 0 || (root.filterIndex === 1) === (modelData.kind === "songs")
                visible: shown; width: results.width; height: shown ? 104 : 0
                radius: 10; color: Theme.secondary
                RowLayout {
                    anchors.fill: parent; anchors.margins: 10; spacing: 12
                    Rectangle {
                        Layout.preferredWidth: 72; Layout.preferredHeight: 72; color: Theme.surface; radius: 8; clip: true
                        SvgIcon { anchors.centerIn: parent; name: "music-note"; color: Theme.iconSecondary }
                        Image { anchors.fill: parent; source: modelData.imageUrl || ""; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize: Qt.size(144, 144) }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 4
                        Text { text: (modelData.kind === "songs" ? "歌曲 · " : "专辑 · ") + modelData.title; color: Theme.text; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: modelData.artist || "未知歌手"; color: Theme.muted; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: modelData.status; color: Theme.muted; Layout.fillWidth: true; elide: Text.ElideRight; ToolTip.visible: statusHover.hovered; ToolTip.text: text; HoverHandler { id: statusHover } }
                    }
                    FutariComboBox {
                        objectName: "coverImageChoice" + coverRow.index
                        Layout.preferredWidth: Math.min(330, root.width * 0.35)
                        model: root.controller.imageNames
                        currentIndex: modelData.imageIndex
                        enabled: !root.controller.busy && !modelData.done
                        Accessible.name: "选择" + modelData.title + "的封面"
                        onActivated: imageIndex => root.controller.selectImage(coverRow.index, imageIndex)
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Text { text: root.controller.total > 0 ? root.controller.completed + " / " + root.controller.total : ""; color: Theme.muted }
            Item { Layout.fillWidth: true }
            FutariButton { text: "关闭"; onClicked: root.close() }
            FutariButton {
                text: "确认补全（" + root.controller.selectedCount + "）"; prominent: true
                enabled: !root.controller.busy && root.controller.selectedCount > 0 && appController.admin
                onClicked: root.controller.submit()
            }
        }
    }
    FolderDialog { id: folderDialog; title: "选择封面图片所在目录"; onAccepted: root.controller.scan(selectedFolder) }
}
