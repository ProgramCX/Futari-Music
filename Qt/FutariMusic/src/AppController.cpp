#include "AppController.h"

#include <QDateTime>

AppController::AppController(QObject* parent)
    : QObject(parent),
      m_api(this),
      m_metadataReader(this),
      m_transfers(this),
      m_player(&m_api, this),
      m_settings(QSettings::defaultFormat(), QSettings::UserScope, "Futari", "FutariMusic") {
    restorePreferences();
    setupConnections();
    setupSocket();
    restoreSession();
}

void AppController::restorePreferences() {
    setServerUrl(m_settings.value("serverUrl", "http://127.0.0.1:8081/").toString());
    m_darkMode = m_settings.value("darkMode", false).toBool();
    m_player.setCacheDirectory(cacheDirectory());
    m_player.setCacheEnabled(cacheEnabled());
    m_player.setVolume(m_settings.value("playerVolume", 0.7).toReal());
    m_username = m_settings.value("username").toString();
    m_autoLogin = m_settings.value("autoLogin", false).toBool();
}

void AppController::setupConnections() {
    connect(&m_player, &AudioPlayer::volumeChanged, this,
            [this] { m_settings.setValue("playerVolume", m_player.volume()); });
    connect(&m_api, &ApiClient::errorOccurred, this, &AppController::setError);
    connect(&m_api, &ApiClient::errorOccurred, this, [this] { m_session.coverLoading.clear(); });
    connect(&m_api, &ApiClient::unauthorized, this, &AppController::clearSession);
    connect(&m_metadataReader, &AudioMetadataReader::metadataReady, this,
            &AppController::audioMetadataReady);
    connect(&m_transfers, &TransferManager::tasksChanged, this,
            &AppController::transferTasksChanged);
    connect(&m_transfers, &TransferManager::unauthorized, this, &AppController::clearSession);
    connect(&m_transfers, &TransferManager::taskSucceeded, this,
            [this](const QString&, const QVariantMap&, const QString& kind) {
                if (kind == "upload") refreshUploadedSongs();
                emit infoMessage(kind == "upload" ? QStringLiteral("歌曲上传完成")
                                                  : QStringLiteral("歌曲下载完成"));
            });
    connect(&m_player, &AudioPlayer::errorOccurred, this, &AppController::setError);
    connect(&m_player, &AudioPlayer::songChanged, this,
            [this] { loadLyrics(m_player.song().value("id").toLongLong()); });
    connect(&m_player, &AudioPlayer::reachedEnd, this, [this] {
        if (!m_session.roomId || roomOwner()) nextSong();
    });
}

void AppController::setupSocket() {
    m_reconnect.setInterval(3000);
    m_reconnect.setSingleShot(true);
    connect(&m_reconnect, &QTimer::timeout, this, &AppController::connectSocket);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &AppController::handleSocketMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, [this] {
        m_heartbeat.stop();
        if (authenticated()) m_reconnect.start();
    });
    connect(&m_socket, &QWebSocket::connected, this, [this] {
        m_heartbeat.start();
        sendSocket("PING", {{"clientTime", QDateTime::currentMSecsSinceEpoch()}});
        refreshInvites();
        if (m_session.roomId) refreshRoomState();
    });
    m_heartbeat.setInterval(20000);
    connect(&m_heartbeat, &QTimer::timeout, this,
            [this] { sendSocket("PING", {{"clientTime", QDateTime::currentMSecsSinceEpoch()}}); });
}

void AppController::restoreSession() {
    const QString savedToken = m_autoLogin ? m_settings.value("token").toString() : QString();
    if (savedToken.isEmpty()) return;
    m_api.setToken(savedToken);
    m_api.request(
        "GET", "api/auth/me", {},
        [this](const QJsonValue& value) { applySession(value.toObject()); },
        [this] { m_api.setToken({}); });
}

void AppController::applySession(const QJsonObject& data) {
    m_session.userId = data.value("userId").toInteger();
    m_session.role = data.value("role").toString();
    m_session.canUpload = data.value("canUpload").toBool();
    m_session.canManageServerPlaylist = data.value("canManageServerPlaylist").toBool();
    m_username = data.value("username").toString();
    m_session.authenticated = true;
    m_transfers.setContext(m_api.baseUrl(), m_api.token(), m_session.userId);
    emit sessionChanged();
    refreshAll();
    leavePreviousRoom();
    connectSocket();
}

QString AppController::segment(const QString& value) {
    return QString::fromLatin1(QUrl::toPercentEncoding(value));
}

void AppController::setServerUrl(const QString& url) {
    const QUrl parsed(url.trimmed());
    if (!parsed.isValid() || (parsed.scheme() != "http" && parsed.scheme() != "https") ||
        parsed.host().isEmpty()) {
        setError(QStringLiteral("服务器地址需为有效的 http 或 https URL"));
        return;
    }
    m_api.setBaseUrl(parsed);
    m_transfers.setContext(m_api.baseUrl(), m_api.token(), m_session.userId);
    m_settings.setValue("serverUrl", m_api.baseUrl().toString());
    emit serverUrlChanged();
}

void AppController::setDarkMode(bool value) {
    if (m_darkMode == value) return;
    m_darkMode = value;
    m_settings.setValue("darkMode", value);
    emit darkModeChanged();
}
void AppController::setAutoLogin(bool value) {
    if (m_autoLogin == value) return;
    m_autoLogin = value;
    m_settings.setValue("autoLogin", value);
    if (value && authenticated())
        m_settings.setValue("token", m_api.token());
    else if (!value)
        m_settings.remove("token");
    emit autoLoginChanged();
}
void AppController::setError(const QString& message) {
    m_errorMessage = message;
    emit errorMessageChanged();
}
void AppController::clearError() { setError({}); }

void AppController::leavePreviousRoom() {
    if (!authenticated()) return;
    m_api.request("GET", "api/rooms/current", {}, [this](const QJsonValue& value) {
        const qint64 previousRoomId =
            value.toObject().value("room").toObject().value("id").toInteger();
        if (previousRoomId <= 0) return;
        m_api.request("POST", "api/rooms/" + QString::number(previousRoomId) + "/leave", {},
                      [this](const QJsonValue&) { refreshRooms(); });
    });
}

void AppController::login(const QString& username, const QString& password) {
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        setError(QStringLiteral("请输入账号和密码"));
        return;
    }
    m_api.request("POST", "api/auth/login",
                  {{"username", username.trimmed()}, {"password", password}},
                  [this, username](const QJsonValue& value) {
                      QJsonObject data = value.toObject();
                      data.insert("username", username.trimmed());
                      m_api.setToken(data.value("token").toString());
                      m_settings.setValue("username", username.trimmed());
                      if (m_autoLogin) m_settings.setValue("token", m_api.token());
                      clearError();
                      applySession(data);
                  });
}
void AppController::registerAccount(const QString& username, const QString& nickname,
                                    const QString& password) {
    m_api.request("POST", "api/auth/register",
                  {{"username", username.trimmed()},
                   {"nickname", nickname.trimmed()},
                   {"password", password}},
                  [this, username, password](const QJsonValue&) { login(username, password); });
}
void AppController::logout() {
    QNetworkReply* logoutReply =
        authenticated() ? m_api.request("POST", "api/auth/logout", {}, {}) : nullptr;
    resetSession(logoutReply);
}
void AppController::clearSession() { resetSession(nullptr); }
void AppController::resetSession(QNetworkReply* logoutReply) {
    // 先使请求域和身份失效，再停服务；close/abort 可能同步触发信号。
    m_session = SessionState{};
    m_transfers.setContext(m_api.baseUrl(), {}, 0);
    m_reconnect.stop();
    m_heartbeat.stop();
    m_socket.close();
    m_player.stop();
    m_api.abortAll(logoutReply);
    m_api.setToken({});
    m_settings.remove("token");
    notifySessionReset();
}

void AppController::notifySessionReset() {
    emit roomDeletionBusyChanged();
    emit playlistCreationBusyChanged();
    emit sessionChanged();
    emit roomStateChanged();
    emit localQueueChanged();
    emit invitesChanged();
    emit songsChanged();
    emit roomsChanged();
    emit partnersChanged();
    emit userResultsChanged();
    emit playlistsChanged();
    emit serverPlaylistsChanged();
    emit adminUsersChanged();
    emit lyricsChanged();
    emit coversChanged();
    emit albumsChanged();
    emit albumMatchesChanged();
    emit uploadedSongsChanged();
}

void AppController::refreshAll() {
    searchSongs({});
    refreshAlbums();
    refreshUploadedSongs();
    refreshRooms();
    refreshPartners();
    refreshPlaylists();
    refreshServerPlaylists();
    refreshInvites();
}
