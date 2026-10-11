import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FutariMusic
import "../components"

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 22

        ColumnLayout {
            spacing: 6

            Text {
                text: "管理工具"
                color: Theme.text
                font.pixelSize: 26
                font.weight: Font.DemiBold
            }

            Text {
                text: "维护曲库中的封面与歌词资料。"
                color: Theme.muted
                font.pixelSize: 14
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 860
            spacing: 12

            Text {
                text: "媒体资料维护"
                color: Theme.muted
                font.pixelSize: 13
                font.weight: Font.DemiBold
                Layout.bottomMargin: 2
            }

            ManagementToolItem {
                Layout.fillWidth: true
                iconName: "music-note"
                title: "补全封面"
                description: "为缺少封面的歌曲和专辑匹配图片"
                summaryText: appController.coverCompletion.summary
                enabled: appController.admin
                Accessible.name: "补全封面"
                Accessible.description: description + (summaryText.length > 0 ? "。" + summaryText : "")
                onClicked: completionDialog.open()
            }

            ManagementToolItem {
                Layout.fillWidth: true
                iconName: "lyrics"
                title: "补充歌词"
                description: "匹配并补充曲库中缺少的逐字或逐行歌词"
                summaryText: appController.lyricsCompletion.summary
                enabled: appController.admin
                Accessible.name: "补充歌词"
                Accessible.description: description + (summaryText.length > 0 ? "。" + summaryText : "")
                onClicked: lyricsDialog.open()
            }
        }

        Item { Layout.fillHeight: true }
    }

    CoverCompletionDialog { id: completionDialog; parent: Overlay.overlay }
    LyricsCompletionDialog { id: lyricsDialog; parent: Overlay.overlay }
}
