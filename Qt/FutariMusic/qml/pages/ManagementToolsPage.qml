import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        Text { text: "管理工具"; color: Theme.text; font.pixelSize: 26; font.bold: true }
        FutariButton {
            text: "补全封面"; iconName: "plus"; prominent: true
            enabled: appController.admin
            onClicked: completionDialog.open()
        }
        Text { text: appController.coverCompletion.summary; color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
        FutariButton {
            text: "补充歌词"; iconName: "plus"
            enabled: appController.admin
            onClicked: lyricsDialog.open()
        }
        Text { text: appController.lyricsCompletion.summary; color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Item { Layout.fillHeight: true }
    }
    CoverCompletionDialog { id: completionDialog; parent: Overlay.overlay }
    LyricsCompletionDialog { id: lyricsDialog; parent: Overlay.overlay }
}
