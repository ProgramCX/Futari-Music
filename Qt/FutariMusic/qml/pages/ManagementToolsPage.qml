import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        Text { text: "管理工具"; color: Theme.text; font.pixelSize: 26; font.bold: true }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.muted
            text: "为服务器缺少封面的歌曲和专辑补全图片。先选择本地目录、检查匹配结果，再确认上传。已有封面会保留。"
        }
        FutariButton {
            text: "补全封面"; iconName: "plus"; prominent: true
            enabled: appController.admin
            onClicked: completionDialog.open()
        }
        Text { text: appController.coverCompletion.summary; color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Item { Layout.fillHeight: true }
    }
    CoverCompletionDialog { id: completionDialog; parent: Overlay.overlay }
}
