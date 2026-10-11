#include "AppController.h"

#include <QDateTime>
#include <QJsonDocument>

AppController::AppController(QObject* parent)
    : QObject(parent),
      m_api(this),
      m_coverCompletion(&m_api, this),
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
    if (savePlaybackQueue()) {
        // Restore metadata with IDs so the queue can render before the next library refresh.
        const QJsonObject savedQueue =
            QJsonDocument::fromJson(m_settings.value("playbackQueue").toByteArray()).object();
        cacheSongs(savedQueue.value("songs").toArray());
        QSet<qint64> uniqueIds;
        for (const QJsonValue& value : savedQueue.value("queue").toArray()) {
            const qint64 songId = value.toInteger();
            if (songId > 0 && !uniqueIds.contains(songId)) {
                m_session.localQueue.append(songId);
                uniqueIds.insert(songId);
            }
        }
        const qint64 currentSongId = savedQueue.value("currentSongId").toInteger();
        if (uniqueIds.contains(currentSongId)) {
            const QJsonObject currentSong = findSong(currentSongId);
            if (!currentSong.isEmpty()) {
                // Restore the selected track and seek target without starting a download.
                m_player.restoreSong(currentSong,
                                     qMax<qint64>(0, savedQueue.value("positionMs").toInteger()));
            }
        }
    }
}

void AppController::setupConnections() {
    connect(&m_coverCompletion, &CoverCompletionController::errorOccurred, this, &AppController::setError);
    connect(this, &AppController::sessionChanged, this,
            [this] { m_coverCompletion.reset(authenticated() && admin()); });
    connect(this, &AppController::serverUrlChanged, this,
            [this] { m_coverCompletion.reset(authenticated() && admin()); });
    connect(&m_coverCompletion, &CoverCompletionController::completionFinished, this,
            &AppController::onCoversCompleted);
    connect(this, &AppController::roomStateChanged, this, &AppController::playbackModeChanged);
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
    connect(&m_player, &AudioPlayer::reachedEnd, this, &AppController::onTrackEnded);
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
void AppController::setMinimizeToTrayOnClose(bool minimize) {
    if (minimizeToTrayOnClose() == minimize) return;
    m_settings.setValue("minimizeToTrayOnClose", minimize);
    emit settingsChanged();
}
void AppController::setSavePlaybackQueue(bool enabled) {
    if (savePlaybackQueue() == enabled) return;
    m_settings.setValue("savePlaybackQueue", enabled);
    if (enabled)
        persistPlaybackQueue();
    else
        m_settings.remove("playbackQueue");
    emit settingsChanged();
}

void AppController::persistPlaybackQueue() {
    if (!savePlaybackQueue()) {
        m_settings.remove("playbackQueue");
        return;
    }
    QJsonArray queue;
    QJsonArray songs;
    // Room playback has its own queue; preserve the personal track captured on entry.
    const QJsonObject currentSong = m_session.roomId
        ? m_session.savedLocalSong : QJsonObject::fromVariantMap(m_player.song());
    const qint64 currentSongId = currentSong.value("id").toInteger();
    const qint64 currentPosition = m_session.roomId
        ? m_session.savedLocalPosition : m_player.position();
    bool currentSongQueued = false;
    // Keep only records needed to render and replay queued items; covers remain in the cover cache.
    for (const QJsonValue& value : m_session.localQueue) {
        const qint64 songId = value.toInteger();
        queue.append(songId);
        const bool isCurrent = songId == currentSongId;
        currentSongQueued |= isCurrent;
        const QJsonObject song = isCurrent ? currentSong : findSong(songId);
        if (!song.isEmpty()) songs.append(song);
    }
    m_settings.setValue("playbackQueue", QJsonDocument(QJsonObject{
        {"queue", queue}, {"songs", songs},
        {"currentSongId", currentSongQueued ? currentSongId : 0},
        {"positionMs", currentSongQueued ? currentPosition : 0}})
        .toJson(QJsonDocument::Compact));
    m_settings.sync();
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
