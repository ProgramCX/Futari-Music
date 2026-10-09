# QML-facing facade; implementations are grouped by business domain.
set(FUTARI_CONTROLLER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/AppController.h
    ${CMAKE_CURRENT_LIST_DIR}/AppController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerLibrary.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerRooms.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerPlayback.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerPlaylists.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerSocial.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerCache.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AppControllerSocket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/models/SessionState.h
    ${CMAKE_CURRENT_LIST_DIR}/models/PageState.h
    ${CMAKE_CURRENT_LIST_DIR}/utils/RequestScope.h
)
