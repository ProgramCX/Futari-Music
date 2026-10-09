cmake_minimum_required(VERSION 3.21)
project(FutariMusic VERSION 0.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FUTARI_CLIENT_VERSION "0.0.1-dev" CACHE STRING "Client version embedded in the executable")
set(FUTARI_SERVER_COMPAT_VERSION "0.1.0" CACHE STRING "Server compatibility version embedded in the executable")
set(FUTARI_WINDOWS_INSTALL_ARGUMENTS "/S" CACHE STRING "Silent installer arguments for Windows update EXE packages")
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Network WebSockets Widgets Svg)
qt_standard_project_setup(REQUIRES 6.8)

include(src/ControllerSources.cmake)

qt_add_executable(appFutariMusic
    main.cpp
    src/ApiClient.h src/ApiClient.cpp
    src/MultipartRequest.h
    src/AudioPlayer.h src/AudioPlayer.cpp
    src/AudioMetadataReader.h src/AudioMetadataReader.cpp
    src/TransferManager.h src/TransferManager.cpp
    ${FUTARI_CONTROLLER_SOURCES}
    src/NotificationService.h src/NotificationService.cpp
    src/ClientUpdateManager.h src/ClientUpdateManager.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(appFutariMusic
    URI FutariMusic
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/Sidebar.qml
        qml/components/PlayerBar.qml
        qml/components/SongRow.qml
        qml/components/BatchSongToolbar.qml
        qml/components/QueueDrawer.qml
        qml/components/SvgIcon.qml
        qml/components/FutariButton.qml
        qml/components/FutariToolButton.qml
        qml/components/FutariTextField.qml
        qml/components/FutariCheckBox.qml
        qml/components/FutariComboBox.qml
        qml/components/PopupSurface.qml
        qml/components/FutariMenu.qml
        qml/components/FutariMenuItem.qml
        qml/components/FutariMenuSeparator.qml
        qml/components/FutariDialog.qml
        qml/components/UploadMenuButton.qml
        qml/pages/LoginPage.qml
        qml/pages/LibraryPage.qml
        qml/pages/RoomPage.qml
        qml/pages/PartnerPage.qml
        qml/pages/PlaylistPage.qml
        qml/pages/MessagesPage.qml
        qml/pages/LyricsPage.qml
        qml/pages/AdminPage.qml
        qml/pages/AccountPage.qml
        qml/pages/SongEditPage.qml
        qml/pages/TransferPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/SingleUploadDialog.qml
        qml/pages/BatchUploadDialog.qml
    RESOURCES
        qml/assets/icons/music-note.svg
        qml/assets/icons/room.svg
        qml/assets/icons/heart.svg
        qml/assets/icons/heart-filled.svg
        qml/assets/icons/play-next.svg
        qml/assets/icons/playlist.svg
        qml/assets/icons/lyrics.svg
        qml/assets/icons/account.svg
        qml/assets/icons/message.svg
        qml/assets/icons/sun.svg
        qml/assets/icons/moon.svg
        qml/assets/icons/window-minimize.svg
        qml/assets/icons/window-maximize.svg
        qml/assets/icons/window-restore.svg
        qml/assets/icons/window-close.svg
        qml/assets/icons/expand.svg
        qml/assets/icons/play.svg
        qml/assets/icons/pause.svg
        qml/assets/icons/previous.svg
        qml/assets/icons/next.svg
        qml/assets/icons/queue-up.svg
        qml/assets/icons/queue-down.svg
        qml/assets/icons/remove.svg
        qml/assets/icons/plus.svg
        qml/assets/icons/chevron-down.svg
        qml/assets/icons/more.svg
        qml/assets/icons/queue.svg
        qml/assets/icons/volume.svg
        qml/assets/icons/check.svg
        qml/assets/icons/admin.svg
        qml/assets/icons/search.svg
        qml/assets/icons/upload.svg
        qml/assets/icons/logout.svg
        qml/assets/icons/trash.svg
        qml/assets/icons/download.svg
        qml/assets/icons/settings.svg
        qml/assets/icons/arrow-left.svg
        qml/assets/icons/arrow-right.svg
)

qt_add_resources(appFutariMusic appIconResources
    PREFIX "/icons"
    FILES icons/futari_icon_1024.png
)

set_target_properties(appFutariMusic PROPERTIES WIN32_EXECUTABLE TRUE)
if(WIN32)
    target_sources(appFutariMusic PRIVATE icons/futari_icon.rc)
elseif(UNIX AND NOT APPLE)
    install(TARGETS appFutariMusic RUNTIME DESTINATION bin)
    install(FILES icons/futari_icon_1024.png
        DESTINATION share/icons/hicolor/1024x1024/apps
        RENAME futari-music.png
    )
    install(FILES icons/futari-music.desktop DESTINATION share/applications)
endif()

target_compile_definitions(appFutariMusic PRIVATE
    "FUTARI_CLIENT_VERSION=\"${FUTARI_CLIENT_VERSION}\""
    "FUTARI_SERVER_COMPAT_VERSION=\"${FUTARI_SERVER_COMPAT_VERSION}\""
    "FUTARI_WINDOWS_INSTALL_ARGUMENTS=\"${FUTARI_WINDOWS_INSTALL_ARGUMENTS}\""
)
target_link_libraries(appFutariMusic PRIVATE
    Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Network
    Qt6::WebSockets Qt6::Widgets Qt6::Svg
)
