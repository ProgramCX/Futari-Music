pragma Singleton
import QtQuick

QtObject {
    readonly property bool dark: appController.darkMode
    readonly property color background: dark ? "#111319" : "#f5f6f8"
    readonly property color surface: dark ? "#1b1e25" : "#ffffff"
    readonly property color secondary: dark ? "#252932" : "#e9ecf0"
    readonly property color hover: dark ? "#303541" : "#e2e6eb"
    readonly property color text: dark ? "#f6f7fa" : "#18202b"
    readonly property color muted: dark ? "#a5adba" : "#657181"
    readonly property color accent: "#13ba78"
    readonly property color accentText: "#ffffff"
    readonly property color iconPrimary: text
    readonly property color iconSecondary: muted
    readonly property color iconAccent: accent
    readonly property color iconOnAccent: accentText
    readonly property color iconDisabled: dark ? "#626975" : "#a5adb8"
    readonly property color buttonFill: secondary
    readonly property color buttonHover: hover
    readonly property color buttonPressed: border
    readonly property color buttonDisabledFill: dark ? "#20232a" : "#eef0f3"
    readonly property color buttonText: text
    readonly property color buttonDisabledText: muted
    readonly property color buttonProminentText: accentText
    readonly property color fieldFill: dark ? "#252932" : "#ffffff"
    readonly property color fieldBorder: border
    readonly property color fieldFocusBorder: accent
    readonly property color checkFill: dark ? "#20242c" : "#ffffff"
    readonly property color checkBorderHover: dark ? "#7e8795" : "#9aa4b2"
    readonly property color checkDisabledFill: dark ? "#303540" : "#edf0f3"
    readonly property color checkDisabledMark: dark ? "#c2c8d1" : "#5e6978"
    readonly property color authBackground: "#211b2d"
    readonly property color authSurface: "#231c30"
    readonly property color authField: "#3b3550"
    readonly property color authBorder: "#51465f"
    readonly property color authText: "#f6f3fa"
    readonly property color authMuted: "#b9b1c7"
    readonly property color authPlaceholder: "#a9a3b6"
    readonly property color authAccent: "#4bdfa0"
    readonly property color authAction: "#2e5cb8"
    readonly property color authLink: "#8db6ff"
    readonly property color border: dark ? "#363c47" : "#e5e9ee"
    readonly property color danger: "#e45d68"
    readonly property color popupShadow: "#000000"
    readonly property color modalScrim: dark ? "#80000000" : "#40000000"
    readonly property int radius: 16
    readonly property int radiusLarge: 20
    readonly property int radiusSmall: 11
    readonly property int spacing: 16
    readonly property int motionFast: 100
    readonly property int colorTransition: 140
}
