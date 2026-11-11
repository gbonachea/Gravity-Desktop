cmake_minimum_required(VERSION 3.21)

project(StellarShell
    VERSION 0.1.0
    DESCRIPTION "A glassmorphism / Windows XP remastered desktop shell for Wayland"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml Network DBus)
qt_standard_project_setup(REQUIRES 6.5)

# ---------------------------------------------------------------------------
# Optional wlr-layer-shell integration (LayerShellQt, used by KWin/Sway/etc.)
# ---------------------------------------------------------------------------
find_package(LayerShellQt QUIET)

# ---------------------------------------------------------------------------
# Wayland client + foreign-toplevel-management protocol
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WAYLAND_CLIENT QUIET IMPORTED_TARGET wayland-client)
endif()

# PAM: se localiza directamente porque pam.pc declara audit como dependencia
# privada y libaudit-dev puede no estar instalado en el sistema de compilación.
find_path(PAM_INCLUDE_DIR security/pam_appl.h REQUIRED)
find_library(PAM_LIBRARY pam REQUIRED)
add_library(PAM::PAM UNKNOWN IMPORTED)
set_target_properties(PAM::PAM PROPERTIES
    IMPORTED_LOCATION "${PAM_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PAM_INCLUDE_DIR}")
set(STELLAR_PAM_TARGET PAM::PAM)

find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)

set(STELLAR_HAVE_FOREIGN_TOPLEVEL 0)
set(STELLAR_FT_HEADER "")
set(STELLAR_FT_SOURCE "")

if(WAYLAND_CLIENT_FOUND AND WAYLAND_SCANNER_EXECUTABLE)
    set(FT_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml")
    set(FT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(STELLAR_FT_HEADER "${FT_GEN_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h")
    set(STELLAR_FT_SOURCE "${FT_GEN_DIR}/wlr-foreign-toplevel-management-unstable-v1-protocol.c")
    file(MAKE_DIRECTORY "${FT_GEN_DIR}")

    add_custom_command(
        OUTPUT "${STELLAR_FT_HEADER}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header "${FT_XML}" "${STELLAR_FT_HEADER}"
        DEPENDS "${FT_XML}"
        COMMENT "Generating Wayland client header (foreign-toplevel-management)"
        VERBATIM)

    add_custom_command(
        OUTPUT "${STELLAR_FT_SOURCE}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code "${FT_XML}" "${STELLAR_FT_SOURCE}"
        DEPENDS "${FT_XML}"
        COMMENT "Generating Wayland protocol code (foreign-toplevel-management)"
        VERBATIM)

    set(STELLAR_HAVE_FOREIGN_TOPLEVEL 1)
    message(STATUS "StellarShell: foreign-toplevel-management enabled")
else()
    message(WARNING
        "StellarShell: libwayland-client or wayland-scanner not found. "
        "The taskbar will compile as an empty model.")
endif()

# ---------------------------------------------------------------------------
# Shared C++ backend
# ---------------------------------------------------------------------------
set(STELLAR_COMMON_SOURCES
    src/common/shellsurface.cpp
    src/common/shellsurface.h
    src/common/iconimageprovider.cpp
    src/common/iconimageprovider.h
    src/common/desktopentrymodel.cpp
    src/common/desktopentrymodel.h
    src/common/desktopiconmodel.cpp
    src/common/desktopiconmodel.h
    src/common/applauncher.cpp
    src/common/applauncher.h
    src/common/filemanager.cpp
    src/common/filemanager.h
    src/common/launchenvironment.cpp
    src/common/launchenvironment.h
    src/common/systemactions.cpp
    src/common/systemactions.h
    src/common/devicescontroller.cpp
    src/common/devicescontroller.h
    src/common/defaultappscontroller.cpp
    src/common/defaultappscontroller.h
    src/common/notificationclient.cpp
    src/common/notificationclient.h
    src/common/notificationserver.cpp
    src/common/notificationserver.h
    src/common/systemstatus.cpp
    src/common/systemstatus.h
    src/common/clock.cpp
    src/common/clock.h
    src/common/storagemonitor.cpp
    src/common/storagemonitor.h
    src/common/weatherservice.cpp
    src/common/weatherservice.h
    src/common/systemhealth.cpp
    src/common/systemhealth.h
    src/common/systemmanager.cpp
    src/common/systemmanager.h
    src/common/wallpapersource.cpp
    src/common/wallpapersource.h
    src/common/sessioninfo.cpp
    src/common/sessioninfo.h
    src/common/settingscontroller.cpp
    src/common/settingscontroller.h
    src/common/menuipc.cpp
    src/common/menuipc.h
    src/common/toplevelmodel.cpp
    src/common/toplevelmodel.h
)

if(STELLAR_HAVE_FOREIGN_TOPLEVEL)
    list(APPEND STELLAR_COMMON_SOURCES "${STELLAR_FT_SOURCE}" "${STELLAR_FT_HEADER}")
endif()

add_library(stellar_common STATIC ${STELLAR_COMMON_SOURCES})

target_include_directories(stellar_common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common"
)

if(STELLAR_HAVE_FOREIGN_TOPLEVEL)
    target_include_directories(stellar_common PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(stellar_common PUBLIC PkgConfig::WAYLAND_CLIENT)
endif()

target_compile_definitions(stellar_common PUBLIC
    STELLAR_HAVE_FOREIGN_TOPLEVEL=${STELLAR_HAVE_FOREIGN_TOPLEVEL}
)

target_link_libraries(stellar_common PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::DBus
)

if(LayerShellQt_FOUND)
    target_link_libraries(stellar_common PUBLIC LayerShellQt::Interface)
    target_compile_definitions(stellar_common PUBLIC STELLAR_HAVE_LAYERSHELLQT=1)
    message(STATUS "StellarShell: LayerShellQt ${LayerShellQt_VERSION} enabled")
else()
    target_compile_definitions(stellar_common PUBLIC STELLAR_HAVE_LAYERSHELLQT=0)
    message(WARNING
        "StellarShell: LayerShellQt not found. The shell will fall back to "
        "regular floating windows. Install layer-shell-qt + liblayershellqtinterface-dev.")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stellar_common PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# QML resources (shared components are compiled into each executable)
# ---------------------------------------------------------------------------
set(STELLAR_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qml")

set(STELLAR_COMMON_QML
    "${STELLAR_QML_DIR}/common/Theme.js"
    "${STELLAR_QML_DIR}/common/GlassCard.qml"
    "${STELLAR_QML_DIR}/common/GlassButton.qml"
    "${STELLAR_QML_DIR}/common/XpFlag.qml"
    "${STELLAR_QML_DIR}/common/StartButton.qml"
    "${STELLAR_QML_DIR}/common/TaskButton.qml"
    "${STELLAR_QML_DIR}/common/WifiGlyph.qml"
    "${STELLAR_QML_DIR}/common/VolumeGlyph.qml"
    "${STELLAR_QML_DIR}/common/BatteryGlyph.qml"
    "${STELLAR_QML_DIR}/common/WeatherGlyph.qml"
    "${STELLAR_QML_DIR}/common/SearchField.qml"
    "${STELLAR_QML_DIR}/common/StellarIcon.qml"
)

function(stellar_add_qml_resources target)
    qt_add_resources(${target} "${target}_qml"
        PREFIX "/qml"
        BASE "${STELLAR_QML_DIR}"
        FILES ${STELLAR_COMMON_QML} ${ARGN}
    )
endfunction()

# ---------------------------------------------------------------------------
# StellarBackground
# ---------------------------------------------------------------------------
qt_add_executable(StellarBackground
    src/background/main.cpp
)
set_target_properties(StellarBackground PROPERTIES OUTPUT_NAME stellar-background)
target_link_libraries(StellarBackground PRIVATE stellar_common Qt6::Quick)
stellar_add_qml_resources(StellarBackground
    "${STELLAR_QML_DIR}/background/Main.qml"
    "${STELLAR_QML_DIR}/background/DesktopIcon.qml"
    "${STELLAR_QML_DIR}/background/DesktopMenu.qml"
)

# ---------------------------------------------------------------------------
# StellarPanel
# ---------------------------------------------------------------------------
qt_add_executable(StellarPanel
    src/panel/main.cpp
)
set_target_properties(StellarPanel PROPERTIES OUTPUT_NAME stellar-panel)
target_link_libraries(StellarPanel PRIVATE stellar_common Qt6::Quick)
stellar_add_qml_resources(StellarPanel
    "${STELLAR_QML_DIR}/panel/Main.qml"
    "${STELLAR_QML_DIR}/panel/TrayPopup.qml"
    "${STELLAR_QML_DIR}/panel/CalendarPopup.qml"
    "${STELLAR_QML_DIR}/panel/NotificationToast.qml"
    "${STELLAR_QML_DIR}/panel/PopupCatcher.qml"
)

# ---------------------------------------------------------------------------
# StellarMenu
# ---------------------------------------------------------------------------
qt_add_executable(StellarMenu
    src/menu/main.cpp
    src/menu/menuwindowcontroller.cpp
    src/menu/menuwindowcontroller.h
)
set_target_properties(StellarMenu PROPERTIES OUTPUT_NAME stellar-menu)
target_link_libraries(StellarMenu PRIVATE stellar_common Qt6::Quick)
stellar_add_qml_resources(StellarMenu
    "${STELLAR_QML_DIR}/menu/Main.qml"
    "${STELLAR_QML_DIR}/menu/Catcher.qml"
    "${STELLAR_QML_DIR}/menu/AppListDelegate.qml"
    "${STELLAR_QML_DIR}/menu/QuickLink.qml"
    "${STELLAR_QML_DIR}/menu/PowerButton.qml"
    "${STELLAR_QML_DIR}/menu/RunDialog.qml"
    "${STELLAR_QML_DIR}/menu/SystemPanel.qml"
)

# ---------------------------------------------------------------------------
# StellarSettings
# ---------------------------------------------------------------------------
qt_add_executable(StellarSettings
    src/settings/main.cpp
)
set_target_properties(StellarSettings PROPERTIES OUTPUT_NAME stellar-settings)
target_link_libraries(StellarSettings PRIVATE stellar_common Qt6::Quick)
stellar_add_qml_resources(StellarSettings
    "${STELLAR_QML_DIR}/settings/Main.qml"
)

# ---------------------------------------------------------------------------
# StellarWidgets
# ---------------------------------------------------------------------------
qt_add_executable(StellarWidgets
    src/widgets/main.cpp
)
set_target_properties(StellarWidgets PROPERTIES OUTPUT_NAME stellar-widgets)
target_link_libraries(StellarWidgets PRIVATE stellar_common Qt6::Quick)
stellar_add_qml_resources(StellarWidgets
    "${STELLAR_QML_DIR}/widgets/Main.qml"
    "${STELLAR_QML_DIR}/widgets/ClockWeatherCard.qml"
    "${STELLAR_QML_DIR}/widgets/StorageCard.qml"
    "${STELLAR_QML_DIR}/widgets/SystemStatusCard.qml"
)

# ---------------------------------------------------------------------------
# StellarLock
# ---------------------------------------------------------------------------
qt_add_executable(StellarLock
    src/lock/main.cpp
    src/lock/lockcontroller.cpp
    src/lock/lockcontroller.h
)
set_target_properties(StellarLock PROPERTIES OUTPUT_NAME stellar-lock)
target_link_libraries(StellarLock PRIVATE stellar_common Qt6::Quick ${STELLAR_PAM_TARGET})
stellar_add_qml_resources(StellarLock
    "${STELLAR_QML_DIR}/lock/Main.qml"
)

# ---------------------------------------------------------------------------
# StellarNotifications (demonio D-Bus)
# ---------------------------------------------------------------------------
qt_add_executable(StellarNotifications
    src/notifications/main.cpp
)
set_target_properties(StellarNotifications PROPERTIES OUTPUT_NAME stellar-notifications)
target_link_libraries(StellarNotifications PRIVATE stellar_common Qt6::Core Qt6::DBus)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS StellarBackground StellarPanel StellarMenu StellarWidgets StellarSettings StellarLock StellarNotifications
    RUNTIME DESTINATION bin)

install(PROGRAMS scripts/run-stellar.sh scripts/stellar-ui scripts/stellar-session
    scripts/stellar-session-labwc scripts/stellar-session-kwin
    scripts/stellar-session-wayfire scripts/stellar-minimize
    DESTINATION bin)

# El tema de SDDM se empaqueta por separado (stellar-sddm-theme), de modo que
# se pueda instalar o quitar sin arrastrar todo el escritorio.

# Temas de iconos incluidos. DamaDamas es el predeterminado.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Apariencia/DamaDamas"
    DESTINATION share/icons
    USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Apariencia/Crystalline"
    DESTINATION share/icons
    USE_SOURCE_PERMISSIONS)
install(PROGRAMS autostart.sh DESTINATION bin RENAME stellar-autostart)
install(PROGRAMS autostart-wayfire.sh DESTINATION bin RENAME stellar-autostart-wayfire)
install(PROGRAMS autostart-labwc.sh DESTINATION bin RENAME stellar-autostart-labwc)
install(PROGRAMS autostart-kwin.sh DESTINATION bin RENAME stellar-autostart-kwin)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "StellarShell configuration:")
message(STATUS "  Qt version ............ ${Qt6_VERSION}")
message(STATUS "  LayerShellQt .......... ${LayerShellQt_FOUND}")
message(STATUS "  foreign-toplevel ...... ${STELLAR_HAVE_FOREIGN_TOPLEVEL}")
message(STATUS "  Build type ............ ${CMAKE_BUILD_TYPE}")
