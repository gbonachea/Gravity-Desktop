.pragma library

// Stellar Shell design tokens -------------------------------------------------
// Colors use #AARRGGBB. Alpha is deliberately moderate: the compositor can
// add real blur behind layer surfaces; the QML gradients provide the acrylic
// fallback when no blur protocol is available.

var fontFamily = "Segoe UI"
var fontFamilyMono = "Cascadia Mono"

var radius = 16
var radiusSmall = 12
var panelHeight = 64
var panelMargin = 12

// Text on blue glass
var textPrimary = "#ffffff"
var textSecondary = "#e8f2ff"
var textMuted = "#c6dcf7"
var textOnLight = "#102a49"
var textOnLightSecondary = "#3f638c"

// Blue acrylic (panel + menu) — algo más opaco para que el texto se lea.
var panelTop = "#d0174b8f"
var panelBottom = "#e00d2a5e"
var panelBorder = "#88c8e6ff"
var panelHighlight = "#40ffffff"

var menuTop = "#e0174a8b"
var menuBottom = "#ee0b2452"
var menuBorder = "#8fcfe7ff"

// White/blue acrylic (widgets)
var lightTop = "#b8ffffff"
var lightBottom = "#7ad8edff"
var lightBorder = "#ccffffff"
var lightHighlight = "#aaffffff"
var lightShadow = "#3d0b2d5a"

var accent = "#59aaff"
var accentStrong = "#2a7cf0"
var accentDeep = "#0c3b8a"
var accentPale = "#cfe6ff"
var xpGreen = "#42c46e"
var xpRed = "#ef5350"
var xpYellow = "#ffd54f"

var shadow = "#55040f22"
var divider = "#2e8fc7ff"
var hover = "#26ffffff"
var pressed = "#3dffffff"

// Animation
var fast = 120
var normal = 180
var slow = 260

function clamp(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value))
}
