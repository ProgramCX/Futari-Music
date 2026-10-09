#include "AppController.h"

void AppController::refreshRooms() {
    resetPage(m_session.rooms, {});
    emit roomsChanged();
    loadMoreRooms();
}

void AppController::loadMoreRooms() {
    loadPage(m_session.rooms, "api/rooms", [this] { emit roomsChanged(); });
}

void AppController::createRoom(const QString& name) {
    if (name.trimmed().isEmpty()) return;
    m_api.request("POST", "api/rooms", {{"name", name.trimmed()}}, [this](const QJsonValue& value) {
        const qint64 id = value.toObject().value("id").toInteger();
        refreshRooms();
        if (id) enterRoom(id);
    });
}

void AppController::joinRoom(qint64 id) {
    m_api.request("POST", "api/rooms/" + QString::number(id) + "/join", {},
                  [this, id](const QJsonValue&) { enterRoom(id); });
}

void AppController::leaveRoom() {
    if (!m_session.roomId) return;
    const qint64 id = m_session.roomId;
    m_api.request("POST", "api/rooms/" + QString::number(id) + "/leave", {},
                  [this, id](const QJsonValue&) {
                      if (m_session.roomId == id) exitRoom();
                      refreshRooms();
                  });
}

void AppController::leaveRoomBeforeClose() {
    if (!m_session.roomId) {
        emit roomLeaveForCloseFinished();
        return;
    }
    const qint64 id = m_session.roomId;
    m_api.request(
        "POST", "api/rooms/" + QString::number(id) + "/leave", {},
        [this, id](const QJsonValue&) {
            if (m_session.roomId == id) exitRoom();
            emit roomLeaveForCloseFinished();
        },
        [this] { emit roomLeaveForCloseFinished(); });
}

void AppController::exitRoom() {
    m_session.roomId = 0;
    m_session.roomState = {};
    m_player.stop();
    if (m_session.savedLocalSong.contains("localPath")) {
        m_player.playLocalFile(m_session.savedLocalSong.value("localPath").toString(),
                               m_session.savedLocalSong.value("title").toString(),
                               m_session.savedLocalSong.value("artist").toString());
        m_player.pause();
        m_player.seek(m_session.savedLocalPosition);
    } else if (!m_session.savedLocalSong.isEmpty())
        m_player.playSong(m_session.savedLocalSong, m_session.savedLocalPosition, false);
    m_session.savedLocalSong = {};
    m_session.savedLocalPosition = 0;
    emit roomStateChanged();
}

bool AppController::canDeleteRoom(qint64 roomId) const {
    if (!authenticated() || roomId <= 0) return false;
    if (m_session.roomId == roomId) return roomOwner();
    for (const auto& entry : m_session.rooms.items) {
        const auto room = entry.toObject();
        if (room.value("id").toInteger() == roomId)
            return room.value("ownerId").toInteger() == m_session.userId;
    }
    return false;
}

void AppController::deleteRoom(qint64 roomId) {
    if (roomDeletionBusy()) return;
    if (!canDeleteRoom(roomId)) {
        setError(QStringLiteral("仅房间创建者可以解散房间"));
        return;
    }
    m_session.deletingRoomId = roomId;
    emit roomDeletionBusyChanged();
    m_api.request(
        "DELETE", "api/rooms/" + QString::number(roomId), {},
        [this, roomId](const QJsonValue&) {
            if (m_session.deletingRoomId != roomId) return;
            if (m_session.roomId == roomId) exitRoom();
            m_session.deletingRoomId = 0;
            emit roomDeletionBusyChanged();
            refreshRooms();
            emit infoMessage(QStringLiteral("房间已解散"));
        },
        [this, roomId] {
            if (m_session.deletingRoomId == roomId) {
                m_session.deletingRoomId = 0;
                emit roomDeletionBusyChanged();
            }
        });
}

void AppController::refreshRoomState() {
    if (!m_session.roomId) return;
    const qint64 id = m_session.roomId;
    m_api.request("GET", "api/rooms/" + QString::number(id) + "/state", {},
                  [this, id](const QJsonValue& value) {
                      if (m_session.roomId == id) setRoomState(value.toObject());
                  });
}

void AppController::setRoomState(const QJsonObject& state) {
    // REST 全量状态可能晚于播放广播返回，保留更新的播放位置。
    const QJsonObject currentPlayback = m_session.roomState.value("playback").toObject();
    m_session.roomState = state;
    if (currentPlayback.value("serverTimestamp").toInteger() >
        state.value("playback").toObject().value("serverTimestamp").toInteger())
        m_session.roomState.insert("playback", currentPlayback);
    emit roomStateChanged();
    applyPlayback(m_session.roomState.value("playback").toObject());
}

void AppController::enterRoom(qint64 roomId) {
    m_session.localPlaybackRequests.renew();
    // 房间和本地队列独立；只在首次进入房间时保存个人播放位置。
    if (!m_session.roomId) {
        m_session.savedLocalSong = QJsonObject::fromVariantMap(m_player.song());
        m_session.savedLocalPosition = m_player.position();
    }
    m_player.stop();
    m_session.roomId = roomId;
    m_session.roomState = {};
    emit roomStateChanged();
    refreshRoomState();
}

bool AppController::canControl() const {
    if (!m_session.roomId) return true;
    if (m_session.roomState.value("room").toObject().value("ownerId").toInteger() ==
        m_session.userId)
        return true;
    for (const QJsonValue& id : m_session.roomState.value("controllerIds").toArray())
        if (id.toInteger() == m_session.userId) return true;
    return false;
}

void AppController::setController(qint64 memberId, bool enabled) {
    if (!m_session.roomId) return;
    m_api.request("PUT", "api/rooms/" + QString::number(m_session.roomId) + "/controllers",
                  {{"memberId", memberId}, {"canControl", enabled}},
                  [this](const QJsonValue&) { refreshRoomState(); });
}
