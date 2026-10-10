cmake_minimum_required(VERSION 3.21)
project(PlaybackUiRegression LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Network WebSockets Widgets Svg Test)
include(../../src/ControllerSources.cmake)
add_executable(playbackUiRegression main.cpp
    ${FUTARI_CONTROLLER_SOURCES}
    ../../src/ApiClient.cpp ../../src/ApiClient.h
    ../../src/ClientUpdateManager.cpp ../../src/ClientUpdateManager.h
    ../../src/AudioPlayer.cpp ../../src/AudioPlayer.h
    ../../src/AudioMetadataReader.cpp ../../src/AudioMetadataReader.h
    ../../src/TransferManager.cpp ../../src/TransferManager.h)
target_link_libraries(playbackUiRegression PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Network Qt6::WebSockets Qt6::Widgets Qt6::Svg Qt6::Test)
get_filename_component(FUTARI_SOURCE_DIR "../.." ABSOLUTE)
target_compile_definitions(playbackUiRegression PRIVATE FUTARI_SOURCE_DIR="${FUTARI_SOURCE_DIR}")
target_compile_definitions(playbackUiRegression PRIVATE FUTARI_CLIENT_VERSION="0.0.0-test" FUTARI_SERVER_COMPAT_VERSION="0.1")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/imports/FutariMusic")
configure_file("../../qml/Theme.qml" "imports/FutariMusic/Theme.qml" COPYONLY)
add_custom_target(playbackUiQml ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${FUTARI_SOURCE_DIR}/qml"
            "${CMAKE_CURRENT_BINARY_DIR}/imports/FutariMusic/qml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FUTARI_SOURCE_DIR}/qml/Theme.qml"
            "${CMAKE_CURRENT_BINARY_DIR}/imports/FutariMusic/Theme.qml")
add_dependencies(playbackUiRegression playbackUiQml)
set(TEST_QMLDIR "module FutariMusic\nsingleton Theme 1.0 Theme.qml\n")
file(GLOB COMPONENT_FILES CONFIGURE_DEPENDS "${FUTARI_SOURCE_DIR}/qml/components/*.qml")
foreach(COMPONENT_FILE IN LISTS COMPONENT_FILES)
    get_filename_component(COMPONENT_NAME "${COMPONENT_FILE}" NAME_WE)
    string(APPEND TEST_QMLDIR "${COMPONENT_NAME} 1.0 qml/components/${COMPONENT_NAME}.qml\n")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/imports/FutariMusic/qmldir" "${TEST_QMLDIR}")
enable_testing()
add_test(NAME playbackUiRegression COMMAND playbackUiRegression)
set_tests_properties(playbackUiRegression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
