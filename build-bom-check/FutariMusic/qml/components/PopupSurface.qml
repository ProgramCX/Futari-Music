import QtQuick
import QtQuick.Effects
import FutariMusic

Rectangle {
    radius: Theme.radiusLarge
    color: Theme.surface
    antialiasing: true
    border.width: 1
    border.color: Theme.border
    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Theme.popupShadow
        shadowOpacity: 0.18
        shadowBlur: 0.55
        shadowVerticalOffset: 6
    }
}
