#include "AppController.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QUrlQuery>

void AppController::connectSocket() {
    if (!authenticated() || m_socket.state() == QAbstractSocket::ConnectedState ||
        m_socket.state() == QAbstractSocket::ConnectingState)
        return;
    QUrl url = m_api.baseUrl().resolved(QUrl("ws"));
    url.setScheme(url.scheme() == "https" ? "wss" : "ws");
    QUrlQuery query;
    query.addQueryItem("token", m_api.token());
    url.setQuery(query);
    m_socket.open(url);
}

void AppController::sendSocket(const QString& type, const QJsonObject& data) {
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        setError(QStringLiteral("实时连接尚未建立"));
        return;
    }
    m_socket.sendTextMessage(QString::fromUtf8(
        QJsonDocument(QJsonObject{{"type", type}, {"data", data}}).toJson(QJsonDocument::Compact)));
}

void AppController::handleSocketMessage(const QString& message) {
    const QJsonObject envelope = QJsonDocument::fromJson(message.toUtf8()).object();
    const QString type = envelope.value("type").toString();
    const QJsonObject data = envelope.value("data").toObject();
    if (type == "PONG") {
        updateServerClock(data);
        return;
    }
    if (type == "INVITE") {
        receiveInvite(data);
        return;
    }
    if (type == "PLAY" || type == "PAUSE" || type == "SEEK" || type == "NEXT" || type == "SYNC" ||
        type == "PLAYBACK_MODE" || type == "TRACK_ENDED") {
        if (!m_session.roomId || isStalePlayback(data)) return;
        m_session.roomState.insert("playback", data);
        emit roomStateChanged();
        applyPlayback(data);
        return;
    }
    if (type == "PLAYLIST_UPDATE" || type == "MEMBER_JOIN" || type == "MEMBER_LEAVE" ||
        type == "CONTROL_UPDATE") {
        refreshRoomState();
        return;
    }
    if (type == "PARTNER_STATUS") refreshPartners();
    if (type == "ROOM_DELETED") {
        if (data.value("roomId").toInteger() == m_session.roomId && m_session.roomId != 0) {
            const bool owner = roomOwner();
            exitRoom();
            if (!owner) emit infoMessage(QStringLiteral("房间已被创建者解散"));
        }
        refreshRooms();
        return;
    }
    if (type == "ERROR") setError(data.value("message").toString());
    if (type == "KICKED") {
        clearSession();
        setError(QStringLiteral("账号已被管理员强制下线"));
    }
}

void AppController::updateServerClock(const QJsonObject& data) {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 sent = data.value("echo").toInteger();
    const qint64 server = data.value("serverTime").toInteger();
    if (sent > 0 && server > 0 && now >= sent && now - sent < 2000) {
        const qint64 sample = server - (sent + now) / 2;
        m_session.clockOffsetMs =
            m_session.clockSynced ? (m_session.clockOffsetMs * 3 + sample) / 4 : sample;
        m_session.clockSynced = true;
    }
}

void AppController::receiveInvite(const QJsonObject& data) {
    const QString id = data.value("inviteId").toString();
    bool exists = false;
    for (const QJsonValue& entry : m_session.invites)
        if (entry.toObject().value("inviteId").toString() == id) exists = true;
    if (!exists) {
        m_session.invites.prepend(data);
        emit invitesChanged();
    }
    emit invitationArrived(QStringLiteral("一起听邀请"),
                           data.value("from").toObject().value("nickname").toString() +
                               QStringLiteral(" 邀请你加入「") + data.value("roomName").toString() +
                               QStringLiteral("」"));
}
