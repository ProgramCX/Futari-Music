#include "AppController.h"

void AppController::refreshPartners() {
    m_api.request("GET", "api/partners", {}, [this](const QJsonValue& value) {
        m_session.partners = value.toArray();
        emit partnersChanged();
    });
}

void AppController::searchUsers(const QString& keyword) {
    resetPage(m_session.userResults, keyword.trimmed());
    if (m_session.userResults.keyword.isEmpty()) m_session.userResults.total = 0;
    emit userResultsChanged();
    loadMoreUsers();
}

void AppController::loadMoreUsers() {
    if (m_session.userResults.keyword.isEmpty()) return;
    loadPage(m_session.userResults, "api/users/search", [this] { emit userResultsChanged(); });
}

void AppController::addPartner(qint64 id) {
    m_api.request("POST", "api/partners/" + QString::number(id), {},
                  [this](const QJsonValue&) { refreshPartners(); });
}

void AppController::removePartner(qint64 id) {
    m_api.request("DELETE", "api/partners/" + QString::number(id), {},
                  [this](const QJsonValue&) { refreshPartners(); });
}

void AppController::invitePartner(qint64 id) {
    if (!m_session.roomId) {
        setError(QStringLiteral("请先进入房间"));
        return;
    }
    m_api.request("POST", "api/rooms/" + QString::number(m_session.roomId) + "/invite",
                  {{"partnerId", id}},
                  [this](const QJsonValue&) { emit infoMessage(QStringLiteral("邀请已发送")); });
}

void AppController::refreshInvites() {
    if (!authenticated()) return;
    m_api.request("GET", "api/invites", {}, [this](const QJsonValue& value) {
        m_session.invites = value.toArray();
        emit invitesChanged();
    });
}

void AppController::respondInvite(const QString& id, bool accept) {
    m_api.request("POST", "api/invites/" + segment(id) + (accept ? "/accept" : "/decline"), {},
                  [this, accept](const QJsonValue& value) {
                      refreshInvites();
                      if (accept) {
                          const QJsonObject result = value.toObject();
                          const qint64 roomId =
                              result.value("room").toObject().value("id").toInteger();
                          if (roomId) {
                              enterRoom(roomId);
                              setRoomState(result);
                          } else
                              refreshRooms();
                      }
                  });
}

void AppController::refreshAdminUsers() {
    if (!admin()) return;
    resetPage(m_session.adminUsers, {});
    emit adminUsersChanged();
    loadMoreAdminUsers();
}

void AppController::loadMoreAdminUsers() {
    if (!admin()) return;
    loadPage(m_session.adminUsers, "api/admin/users", [this] { emit adminUsersChanged(); });
}

void AppController::updatePermissions(qint64 id, bool upload, bool serverPlaylist) {
    if (!admin()) return;
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/permissions",
                  {{"canUpload", upload}, {"canManageServerPlaylist", serverPlaylist}},
                  [this](const QJsonValue&) { refreshAdminUsers(); });
}

void AppController::updateRole(qint64 id, const QString& role) {
    if (!admin() || (role != "USER" && role != "ADMIN")) return;
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/role", {{"role", role}},
                  [this](const QJsonValue&) { refreshAdminUsers(); });
}

void AppController::resetPassword(qint64 id, const QString& password) {
    if (!admin() || password.size() < 8) {
        setError(QStringLiteral("新密码至少 8 位"));
        return;
    }
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/password",
                  {{"newPassword", password}},
                  [this](const QJsonValue&) { emit infoMessage(QStringLiteral("密码已重置")); });
}

void AppController::kickUser(qint64 id) {
    if (!admin()) return;
    m_api.request("POST", "api/admin/users/" + QString::number(id) + "/kick", {},
                  [this](const QJsonValue&) {
                      refreshAdminUsers();
                      emit infoMessage(QStringLiteral("已强制用户下线"));
                  });
}
