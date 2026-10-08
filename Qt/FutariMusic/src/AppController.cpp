#include "AppController.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QDirIterator>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStringConverter>

namespace {
QJsonObject object(const QJsonValue &value) { return value.toObject(); }
QJsonArray array(const QJsonValue &value) { return value.toArray(); }
qint64 number(const QJsonValue &value) { return value.toVariant().toLongLong(); }
void appendTextPart(QHttpMultiPart *parts, const QString &name, const QString &value) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + name + "\"");
    part.setBody(value.toUtf8());
    parts->append(part);
}
bool appendFilePart(QHttpMultiPart *parts, const QString &name, const QString &url) {
    if (url.isEmpty()) return true;
    const QString path = QUrl(url).toLocalFile();
    auto *file = new QFile(path, parts);
    if (!file->open(QIODevice::ReadOnly)) { delete file; return false; }
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   "form-data; name=\"" + name + "\"; filename=\"" + QFileInfo(path).fileName() + "\"");
    part.setBodyDevice(file);
    parts->append(part);
    return true;
}
}

AppController::AppController(QObject *parent)
    : QObject(parent), m_api(this), m_metadataReader(this), m_transfers(this), m_player(&m_api, this),
      m_settings(QSettings::defaultFormat(), QSettings::UserScope, "Futari", "FutariMusic") {
    setServerUrl(m_settings.value("serverUrl", "http://127.0.0.1:8081/").toString());
    m_darkMode = m_settings.value("darkMode", false).toBool();
    m_player.setCacheDirectory(cacheDirectory());
    m_player.setCacheEnabled(cacheEnabled());
    m_username = m_settings.value("username").toString();
    m_autoLogin = m_settings.value("autoLogin", false).toBool();
    connect(&m_api, &ApiClient::errorOccurred, this, &AppController::setError);
    connect(&m_api, &ApiClient::errorOccurred, this, [this] { m_coverLoading.clear(); });
    connect(&m_api, &ApiClient::unauthorized, this, &AppController::clearSession);
    connect(&m_metadataReader, &AudioMetadataReader::metadataReady, this, &AppController::audioMetadataReady);
    connect(&m_transfers, &TransferManager::tasksChanged, this, &AppController::transferTasksChanged);
    connect(&m_transfers, &TransferManager::unauthorized, this, &AppController::clearSession);
    connect(&m_transfers, &TransferManager::taskSucceeded, this, [this](const QString &, const QVariantMap &, const QString &kind) {
        if (kind == "upload") refreshUploadedSongs();
        emit infoMessage(kind == "upload" ? QStringLiteral("歌曲上传完成") : QStringLiteral("歌曲下载完成"));
    });
    connect(&m_player, &AudioPlayer::errorOccurred, this, &AppController::setError);
    connect(&m_player, &AudioPlayer::songChanged, this, [this] {
        loadLyrics(m_player.song().value("id").toLongLong());
    });
    connect(&m_player, &AudioPlayer::reachedEnd, this, [this] {
        if (!m_roomId || roomOwner()) nextSong();
    });
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
        if (m_roomId) refreshRoomState();
    });
    m_heartbeat.setInterval(20000);
    connect(&m_heartbeat, &QTimer::timeout, this, [this] {
        sendSocket("PING", {{"clientTime", QDateTime::currentMSecsSinceEpoch()}});
    });
    const QString savedToken = m_autoLogin ? m_settings.value("token").toString() : QString();
    if (!savedToken.isEmpty()) {
        m_api.setToken(savedToken);
        m_api.request("GET", "api/auth/me", {}, [this](const QJsonValue &value) {
            const QJsonObject session = object(value);
            m_userId = number(session.value("userId"));
            m_username = session.value("username").toString();
            m_role = session.value("role").toString();
            m_canUpload = session.value("canUpload").toBool();
            m_canManageServerPlaylist = session.value("canManageServerPlaylist").toBool();
            m_transfers.setContext(m_api.baseUrl(), m_api.token(), m_userId);
            m_authenticated = true; emit sessionChanged();
            refreshAll(); restoreCurrentRoom(); connectSocket();
        }, [this] { m_api.setToken({}); });
    }
}

QString AppController::segment(const QString &value) {
    return QString::fromLatin1(QUrl::toPercentEncoding(value));
}

void AppController::setServerUrl(const QString &url) {
    const QUrl parsed(url.trimmed());
    if (!parsed.isValid() || (parsed.scheme() != "http" && parsed.scheme() != "https") || parsed.host().isEmpty()) {
        setError(QStringLiteral("服务器地址需为有效的 http 或 https URL")); return;
    }
    m_api.setBaseUrl(parsed);
    m_transfers.setContext(m_api.baseUrl(), m_api.token(), m_userId);
    m_settings.setValue("serverUrl", m_api.baseUrl().toString());
    emit serverUrlChanged();
}
QString AppController::downloadDirectory() const {
    const QString configured = m_settings.value("downloadDirectory").toString();
    if (!configured.isEmpty()) return configured;
    QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (music.isEmpty()) music = QDir::homePath() + QStringLiteral("/Music");
    return QDir::cleanPath(music);
}
QString AppController::cacheDirectory() const {
    const QString configured = m_settings.value("cacheDirectory").toString();
    return QDir::cleanPath(configured.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/music_cache")
            : configured);
}
namespace {
bool pathsOverlap(const QString &left, const QString &right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
    const auto contains = [sensitivity](const QString &parent, const QString &child) {
        return QString::compare(parent, child, sensitivity) == 0 || child.startsWith(parent + QDir::separator(), sensitivity);
    };
    return contains(a, b) || contains(b, a);
}
}
void AppController::setDownloadDirectory(const QString &directory) {
    const QUrl selected(directory.trimmed());
    const QString trimmed = selected.isLocalFile() ? selected.toLocalFile() : directory.trimmed();
    if (trimmed.isEmpty()) { setError(QStringLiteral("请选择下载目录")); return; }
    const QString path = QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
    if (!QDir().mkpath(path) || !QFileInfo(path).isWritable()) {
        setError(QStringLiteral("下载目录不可写，请选择其他文件夹")); return;
    }
    if (pathsOverlap(path, cacheDirectory())) {
        setError(QStringLiteral("下载目录不能与歌曲缓存目录重叠")); return;
    }
    m_settings.setValue("downloadDirectory", path); emit settingsChanged();
}
void AppController::setCacheDirectory(const QString &directory) {
    const QUrl selected(directory.trimmed());
    const QString trimmed = selected.isLocalFile() ? selected.toLocalFile() : directory.trimmed();
    if (trimmed.isEmpty()) { setError(QStringLiteral("请选择缓存目录")); return; }
    const QString path = QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
    if (!QDir().mkpath(path) || !QFileInfo(path).isWritable()) {
        setError(QStringLiteral("缓存目录不可写，请选择其他文件夹")); return;
    }
    if (pathsOverlap(path, downloadDirectory())) {
        setError(QStringLiteral("缓存目录不能与下载目录重叠，以免清理缓存时误删下载歌曲")); return;
    }
    m_settings.setValue("cacheDirectory", path);
    m_player.setCacheDirectory(path);
    emit settingsChanged();
}
void AppController::setCacheEnabled(bool enabled) {
    if (cacheEnabled() == enabled) return;
    m_settings.setValue("cacheEnabled", enabled);
    m_player.setCacheEnabled(enabled);
    emit settingsChanged();
}
qint64 AppController::cacheUsageBytes() const {
    qint64 total = 0;
    QDirIterator iterator(cacheDirectory(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) { iterator.next(); if (!iterator.fileInfo().isSymLink()) total += iterator.fileInfo().size(); }
    return total;
}
void AppController::setDarkMode(bool value) {
    if (m_darkMode == value) return;
    m_darkMode = value;
    m_settings.setValue("darkMode", value);
    emit darkModeChanged();
}
void AppController::setAutoLogin(bool value) {
    if (m_autoLogin == value) return;
    m_autoLogin = value; m_settings.setValue("autoLogin", value);
    if (value && authenticated()) m_settings.setValue("token", m_api.token());
    else if (!value) m_settings.remove("token");
    emit autoLoginChanged();
}
void AppController::setError(const QString &message) {
    m_errorMessage = message;
    emit errorMessageChanged();
}
void AppController::clearError() { setError({}); }

void AppController::login(const QString &username, const QString &password) {
    if (username.trimmed().isEmpty() || password.isEmpty()) { setError(QStringLiteral("请输入账号和密码")); return; }
    m_api.request("POST", "api/auth/login", {{"username", username.trimmed()}, {"password", password}},
                  [this, username](const QJsonValue &value) {
        const QJsonObject data = object(value);
        m_api.setToken(data.value("token").toString());
        m_authenticated = true;
        m_userId = number(data.value("userId"));
        m_canUpload = data.value("canUpload").toBool();
        m_canManageServerPlaylist = data.value("canManageServerPlaylist").toBool();
            m_role = data.value("role").toString();
            m_username = username.trimmed();
        m_transfers.setContext(m_api.baseUrl(), m_api.token(), m_userId);
        m_settings.setValue("username", m_username);
        if (m_autoLogin) {
            m_settings.setValue("token", m_api.token());
        }
        clearError(); emit sessionChanged(); refreshAll(); restoreCurrentRoom(); connectSocket();
    });
}
void AppController::registerAccount(const QString &username, const QString &nickname, const QString &password) {
    m_api.request("POST", "api/auth/register", {{"username", username.trimmed()},
                  {"nickname", nickname.trimmed()}, {"password", password}},
                  [this, username, password](const QJsonValue &) { login(username, password); });
}
void AppController::logout() {
    QNetworkReply *logoutReply = authenticated()
            ? m_api.request("POST", "api/auth/logout", {}, {}) : nullptr;
    resetSession(logoutReply);
}
void AppController::clearSession() { resetSession(nullptr); }
void AppController::resetSession(QNetworkReply *logoutReply) {
    m_transfers.setContext(m_api.baseUrl(), {}, 0);
    m_reconnect.stop(); m_socket.close(); m_player.stop(); m_api.abortAll(logoutReply); m_api.setToken({});
    m_roomId = 0; m_userId = 0; m_roomState = {}; m_invites = {}; m_localQueue = {}; m_songCache.clear();
    m_deletingRoomId = 0; emit roomDeletionBusyChanged();
    m_savedLocalSong = {}; m_savedLocalPosition = 0; m_favoriteBusy = false; m_playlistCreationBusy = false;
    ++m_playlistsGeneration; ++m_serverPlaylistsGeneration;
    emit playlistCreationBusyChanged();
    m_songs = {}; m_albums = {}; m_uploadedSongs = {}; m_uploadedSongTotal = 0; m_uploadedSongPage = 0; m_rooms = {}; m_partners = {}; m_userResults = {}; m_playlists = {};
    m_serverPlaylists = {}; m_adminUsers = {}; m_lyrics = {}; ++m_lyricsGeneration; m_albumMatches = {}; m_covers.clear(); m_coverLoading.clear();
    m_songPage = m_roomPage = m_songTotal = m_roomTotal = 0;
    m_userPage = m_userTotal = m_adminPage = m_adminTotal = 0;
    m_songsLoading = m_roomsLoading = m_usersLoading = m_adminLoading = false;
    m_uploadedSongsLoading = false;
    ++m_uploadedSongsGeneration;
    ++m_songSearchGeneration; ++m_roomSearchGeneration;
    ++m_albumSearchGeneration;
    ++m_userSearchGeneration; ++m_adminSearchGeneration;
    m_canUpload = false; m_role.clear();
    m_canManageServerPlaylist = false; m_authenticated = false; m_settings.remove("token");
    emit sessionChanged(); emit roomStateChanged();
    emit localQueueChanged(); emit invitesChanged(); emit songsChanged(); emit roomsChanged();
    emit partnersChanged(); emit userResultsChanged(); emit playlistsChanged();
    emit serverPlaylistsChanged(); emit adminUsersChanged(); emit lyricsChanged(); emit coversChanged(); emit albumsChanged(); emit albumMatchesChanged(); emit uploadedSongsChanged();
}
void AppController::refreshAll() {
    searchSongs({}); refreshAlbums(); refreshUploadedSongs(); refreshRooms(); refreshPartners(); refreshPlaylists(); refreshServerPlaylists(); refreshInvites();
}
void AppController::searchSongs(const QString &keyword) {
    m_songQuery = keyword.trimmed();
    m_songPage = 0; m_songTotal = 1; m_songs = {}; m_songsLoading = false;
    ++m_songSearchGeneration; emit songsChanged(); loadMoreSongs();
}
void AppController::loadMoreSongs() {
    if (m_songsLoading || !hasMoreSongs()) return;
    m_songsLoading = true;
    const quint64 generation = m_songSearchGeneration;
    const int page = m_songPage + 1;
    m_api.request("GET", "api/songs?keyword=" + segment(m_songQuery) + "&pageNum=" + QString::number(page) + "&pageSize=40", {},
                  [this, generation, page](const QJsonValue &value) {
        if (generation != m_songSearchGeneration) return;
        const QJsonObject result = object(value);
        m_songTotal = static_cast<int>(number(result.value("total")));
        for (const QJsonValue &song : result.value("list").toArray()) {
            m_songs.append(song);
            m_songCache.insert(number(song.toObject().value("id")), song.toObject());
        }
        m_songPage = page; m_songsLoading = false;
        emit songsChanged(); fetchCovers();
    }, [this, generation] { if (generation == m_songSearchGeneration) m_songsLoading = false; });
}
void AppController::refreshAlbums(const QString &keyword) {
    m_api.request("GET", "api/albums?keyword=" + segment(keyword.trimmed()) + "&pageNum=1&pageSize=100", {},
                  [this](const QJsonValue &value) {
        m_albums = object(value).value("list").toArray();
        emit albumsChanged();
    });
}
void AppController::refreshUploadedSongs() {
    ++m_uploadedSongsGeneration;
    m_uploadedSongs = {}; m_uploadedSongPage = 0; m_uploadedSongTotal = 0; m_uploadedSongsLoading = false;
    emit uploadedSongsChanged();
    loadMoreUploadedSongs();
}
void AppController::loadMoreUploadedSongs() {
    if (!authenticated() || m_uploadedSongsLoading || m_uploadedSongs.size() >= m_uploadedSongTotal && m_uploadedSongPage > 0) return;
    m_uploadedSongsLoading = true;
    const quint64 generation = m_uploadedSongsGeneration;
    const int page = m_uploadedSongPage + 1;
    m_api.request("GET", "api/songs/mine?pageNum=" + QString::number(page) + "&pageSize=40", {}, [this, generation, page](const QJsonValue &value) {
        if (generation != m_uploadedSongsGeneration) return;
        const QJsonObject result = object(value);
        m_uploadedSongTotal = static_cast<int>(number(result.value("total")));
        for (const QJsonValue &song : result.value("list").toArray()) m_uploadedSongs.append(song);
        m_uploadedSongPage = page; m_uploadedSongsLoading = false; emit uploadedSongsChanged();
    }, [this, generation] { if (generation == m_uploadedSongsGeneration) m_uploadedSongsLoading = false; });
}
QString AppController::enqueueUpload(const QVariantMap &metadata) {
    if (!canUpload()) { setError(QStringLiteral("当前账号没有歌曲上传权限")); return {}; }
    const QString id = m_transfers.enqueueUpload(metadata);
    if (id.isEmpty()) setError(QStringLiteral("无法创建上传任务，请检查音频文件"));
    return id;
}
void AppController::enqueueDownload(const QVariantMap &song) {
    if (!authenticated()) { setError(QStringLiteral("请先登录后下载歌曲")); return; }
    if (m_transfers.enqueueDownload(song, downloadDirectory()).isEmpty()) setError(QStringLiteral("无法创建下载任务，请检查歌曲文件或下载目录"));
}
void AppController::openLocalFolder(const QString &path) {
    const QFileInfo info(path);
    const QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    if (!QFileInfo::exists(folder) || !QDesktopServices::openUrl(QUrl::fromLocalFile(folder)))
        setError(QStringLiteral("无法打开此文件夹"));
}
void AppController::clearSongCache() {
    if (pathsOverlap(cacheDirectory(), downloadDirectory())) { setError(QStringLiteral("缓存目录与下载目录重叠，已停止清理")); return; }
    const QString directoryPath = cacheDirectory();
    QDir directory(directoryPath);
    const QStringList protectedPaths = m_player.protectedCachePaths();
    static const QRegularExpression cacheFilePattern(QStringLiteral("^[a-f0-9]{64}\\.(mp3|flac|aac|ogg|wav|m4a)(\\.part)?$"));
    for (const QFileInfo &file : directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (file.isSymLink() || !cacheFilePattern.match(file.fileName()).hasMatch()) continue;
        bool protectedFile = false;
        for (const QString &path : protectedPaths)
            if (QString::compare(QDir::cleanPath(file.absoluteFilePath()), QDir::cleanPath(path), Qt::CaseInsensitive) == 0) protectedFile = true;
        if (!protectedFile) QFile::remove(file.absoluteFilePath());
    }
    emit settingsChanged(); emit infoMessage(QStringLiteral("已清理未使用的歌曲缓存"));
}
void AppController::searchAlbumMatches(const QString &name, const QString &artist) {
    const quint64 generation = ++m_albumSearchGeneration;
    const QString cleanedName = name.trimmed();
    if (cleanedName.isEmpty()) { m_albumMatches = {}; emit albumMatchesChanged(); return; }
    m_api.request("GET", "api/albums?keyword=" + segment(cleanedName) + "&pageNum=1&pageSize=100", {},
                  [this, cleanedName, artist, generation](const QJsonValue &value) {
        if (generation != m_albumSearchGeneration) return;
        const QJsonArray rows = object(value).value("list").toArray();
        QList<QJsonObject> exact;
        for (const QJsonValue &row : rows) {
            const QJsonObject album = row.toObject();
            if (QString::compare(album.value("name").toString(), cleanedName, Qt::CaseInsensitive) == 0) exact.append(album);
        }
        const QString cleanedArtist = artist.trimmed();
        if (!cleanedArtist.isEmpty()) {
            std::stable_sort(exact.begin(), exact.end(), [&cleanedArtist](const QJsonObject &left, const QJsonObject &right) {
                const bool leftMatch = QString::compare(left.value("artist").toString(), cleanedArtist, Qt::CaseInsensitive) == 0;
                const bool rightMatch = QString::compare(right.value("artist").toString(), cleanedArtist, Qt::CaseInsensitive) == 0;
                return leftMatch && !rightMatch;
            });
        }
        m_albumMatches = QJsonArray{};
        for (const QJsonObject &album : exact) m_albumMatches.append(album);
        emit albumMatchesChanged();
    });
}
void AppController::inspectAudio(const QString &audioUrl, const QString &requestId) {
    m_metadataReader.inspect(audioUrl, requestId);
}
QVariantMap AppController::findLyricsForAudio(const QString &audioUrl) const {
    const QString audioPath = QUrl(audioUrl).toLocalFile();
    const QFileInfo audioInfo(audioPath);
    if (!audioInfo.isFile()) return {{"found", false}, {"message", QStringLiteral("无法读取所选音频目录")}};
    const QString stem = audioInfo.completeBaseName();
    const QFileInfoList files = QDir(audioInfo.absolutePath()).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &suffix : {QStringLiteral("lrc"), QStringLiteral("txt")}) {
        for (const QFileInfo &file : files) {
            if (file.suffix().compare(suffix, Qt::CaseInsensitive) == 0 &&
                QString::compare(file.completeBaseName(), stem, Qt::CaseInsensitive) == 0) {
                return {{"found", true}, {"url", QUrl::fromLocalFile(file.absoluteFilePath()).toString()}, {"name", file.fileName()}};
            }
        }
    }
    return {{"found", false}, {"message", QStringLiteral("未找到同名歌词文件")}};
}
QString AppController::readLyricsFile(const QString &lyricsUrl) const {
    QFile file(QUrl(lyricsUrl).toLocalFile());
    if (!file.open(QIODevice::ReadOnly) || file.size() > 256 * 1024) return {};
    QByteArray bytes = file.readAll();
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes.remove(0, 3);
    if (bytes.startsWith("\xFF\xFE")) {
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        return decoder.decode(QByteArrayView(bytes).sliced(2));
    }
    if (bytes.startsWith("\xFE\xFF")) {
        QStringDecoder decoder(QStringDecoder::Utf16BE);
        return decoder.decode(QByteArrayView(bytes).sliced(2));
    }
    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString decoded = utf8.decode(bytes);
    if (!utf8.hasError()) return decoded;
    QStringDecoder system(QStringDecoder::System);
    return system.decode(bytes);
}
void AppController::refreshRooms() {
    m_roomPage = 0; m_roomTotal = 1; m_rooms = {}; m_roomsLoading = false;
    ++m_roomSearchGeneration; emit roomsChanged(); loadMoreRooms();
}
void AppController::loadMoreRooms() {
    if (m_roomsLoading || !hasMoreRooms()) return;
    m_roomsLoading = true;
    const quint64 generation = m_roomSearchGeneration;
    const int page = m_roomPage + 1;
    m_api.request("GET", "api/rooms?pageNum=" + QString::number(page) + "&pageSize=40", {}, [this, generation, page](const QJsonValue &value) {
        if (generation != m_roomSearchGeneration) return;
        const QJsonObject result = object(value);
        m_roomTotal = static_cast<int>(number(result.value("total")));
        for (const QJsonValue &room : result.value("list").toArray()) m_rooms.append(room);
        m_roomPage = page; m_roomsLoading = false; emit roomsChanged();
    }, [this, generation] { if (generation == m_roomSearchGeneration) m_roomsLoading = false; });
}
void AppController::createRoom(const QString &name) {
    if (name.trimmed().isEmpty()) return;
    m_api.request("POST", "api/rooms", {{"name", name.trimmed()}}, [this](const QJsonValue &value) {
        const qint64 id = number(object(value).value("id"));
        refreshRooms();
        if (id) enterRoom(id);
    });
}
void AppController::joinRoom(qint64 id) {
    m_api.request("POST", "api/rooms/" + QString::number(id) + "/join", {}, [this, id](const QJsonValue &) {
        enterRoom(id);
    });
}
void AppController::leaveRoom() {
    if (!m_roomId) return;
    const qint64 id = m_roomId;
    m_api.request("POST", "api/rooms/" + QString::number(id) + "/leave", {}, [this, id](const QJsonValue &) {
        if (m_roomId == id) exitRoom();
        refreshRooms();
    });
}
void AppController::exitRoom() {
    m_roomId = 0; m_roomState = {}; m_player.stop();
    if (m_savedLocalSong.contains("localPath")) {
        m_player.playLocalFile(m_savedLocalSong.value("localPath").toString(), m_savedLocalSong.value("title").toString(), m_savedLocalSong.value("artist").toString());
        m_player.pause(); m_player.seek(m_savedLocalPosition);
    } else if (!m_savedLocalSong.isEmpty()) m_player.playSong(m_savedLocalSong, m_savedLocalPosition, false);
    m_savedLocalSong = {}; m_savedLocalPosition = 0;
    emit roomStateChanged();
}
bool AppController::canDeleteRoom(qint64 roomId) const {
    if (!authenticated() || roomId <= 0) return false;
    if (m_roomId == roomId) return roomOwner();
    for (const auto &entry : m_rooms) {
        const auto room = entry.toObject();
        if (number(room.value("id")) == roomId) return number(room.value("ownerId")) == m_userId;
    }
    return false;
}
void AppController::deleteRoom(qint64 roomId) {
    if (roomDeletionBusy()) return;
    if (!canDeleteRoom(roomId)) { setError(QStringLiteral("仅房间创建者可以解散房间")); return; }
    m_deletingRoomId = roomId; emit roomDeletionBusyChanged();
    m_api.request("DELETE", "api/rooms/" + QString::number(roomId), {}, [this, roomId](const QJsonValue &) {
        if (m_deletingRoomId != roomId) return;
        if (m_roomId == roomId) exitRoom();
        m_deletingRoomId = 0; emit roomDeletionBusyChanged();
        refreshRooms(); emit infoMessage(QStringLiteral("房间已解散"));
    }, [this, roomId] {
        if (m_deletingRoomId == roomId) { m_deletingRoomId = 0; emit roomDeletionBusyChanged(); }
    });
}
void AppController::refreshRoomState() {
    if (!m_roomId) return;
    const qint64 id = m_roomId;
    m_api.request("GET", "api/rooms/" + QString::number(id) + "/state", {},
                  [this, id](const QJsonValue &value) { if (m_roomId == id) setRoomState(object(value)); });
}
void AppController::restoreCurrentRoom() {
    m_api.request("GET", "api/rooms/current", {}, [this](const QJsonValue &value) {
        if (m_roomId) return;
        const QJsonObject state = object(value);
        if (state.isEmpty()) return;
        m_roomId = number(state.value("room").toObject().value("id"));
        if (m_roomId) setRoomState(state);
    });
}
void AppController::setRoomState(const QJsonObject &state) {
    // REST 全量状态可能晚于播放广播返回，保留更新的播放位置。
    const QJsonObject currentPlayback = m_roomState.value("playback").toObject();
    m_roomState = state;
    if (number(currentPlayback.value("serverTimestamp")) > number(state.value("playback").toObject().value("serverTimestamp")))
        m_roomState.insert("playback", currentPlayback);
    emit roomStateChanged();
    applyPlayback(m_roomState.value("playback").toObject());
}
void AppController::enterRoom(qint64 roomId) {
    // 房间和本地队列独立；只在首次进入房间时保存个人播放位置。
    if (!m_roomId) {
        m_savedLocalSong = QJsonObject::fromVariantMap(m_player.song());
        m_savedLocalPosition = m_player.position();
    }
    m_player.stop();
    m_roomId = roomId; m_roomState = {};
    emit roomStateChanged();
    refreshRoomState();
}
void AppController::fetchCovers() {
    for (const QJsonValue &value : m_songs) fetchCover(value.toObject());
}
void AppController::fetchCover(const QJsonObject &song) {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
    QDir().mkpath(directory);
        const QString id = QString::number(number(song.value("id")));
        if (song.value("coverUrl").isNull() || m_covers.contains(id) || m_coverLoading.contains(id)) return;
        const QString path = directory + "/" + id + ".image";
        if (QFile::exists(path)) {
            m_covers.insert(id, QUrl::fromLocalFile(path).toString()); emit coversChanged();
            pruneCoverCache(path); return;
        }
        m_coverLoading.insert(id);
        const quint64 requestGeneration = m_coverGeneration;
        m_api.download("api/songs/" + id + "/cover", [this, path, id, requestGeneration](const QJsonValue &encoded) {
            if (requestGeneration != m_coverGeneration) return;
            m_coverLoading.remove(id);
            const QByteArray bytes = QByteArray::fromBase64(encoded.toString().toLatin1());
            QSaveFile file(path);
            if (!bytes.isEmpty() && file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit()) {
                m_covers.insert(id, QUrl::fromLocalFile(path).toString()); emit coversChanged();
                pruneCoverCache(path);
            }
        });
}
void AppController::invalidateCoverCache(qint64 songId) {
    ++m_coverGeneration;
    m_coverLoading.clear();
    const QString directoryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
    if (songId > 0) {
        const QString id = QString::number(songId);
        m_covers.remove(id);
        QFile::remove(directoryPath + "/" + id + ".image");
    } else {
        m_covers.clear();
        QDir directory(directoryPath);
        for (const QString &fileName : directory.entryList({"*.image"}, QDir::Files))
            directory.remove(fileName);
    }
    emit coversChanged();
}
void AppController::pruneCoverCache(const QString &protectedPath) {
    constexpr qint64 maxBytes = 128LL * 1024 * 1024;
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers");
    QFileInfoList files = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 total = 0;
    for (const QFileInfo &file : files) total += file.size();
    std::sort(files.begin(), files.end(), [](const QFileInfo &left, const QFileInfo &right) {
        return left.lastModified() < right.lastModified();
    });
    for (const QFileInfo &file : files) {
        if (total <= maxBytes) break;
        if (file.absoluteFilePath() == protectedPath) continue;
        if (QFile::remove(file.absoluteFilePath())) {
            total -= file.size(); m_covers.remove(file.baseName()); emit coversChanged();
        }
    }
}
bool AppController::canControl() const {
    if (!m_roomId) return true;
    if (number(m_roomState.value("room").toObject().value("ownerId")) == m_userId) return true;
    for (const QJsonValue &id : m_roomState.value("controllerIds").toArray())
        if (number(id) == m_userId) return true;
    return false;
}
void AppController::setController(qint64 memberId, bool enabled) {
    if (!m_roomId) return;
    m_api.request("PUT", "api/rooms/" + QString::number(m_roomId) + "/controllers",
                  {{"memberId", memberId}, {"canControl", enabled}},
                  [this](const QJsonValue &) { refreshRoomState(); });
}
QJsonObject AppController::findSong(qint64 id) const {
    if (m_songCache.contains(id)) return m_songCache.value(id);
    for (const QJsonValue &value : m_songs)
        if (number(value.toObject().value("id")) == id) return value.toObject();
    for (const QJsonValue &playlist : m_playlists)
        for (const QJsonValue &value : playlist.toObject().value("songs").toArray())
            if (number(value.toObject().value("id")) == id) return value.toObject();
    return {};
}
void AppController::playSong(const QVariantMap &song) {
    const QJsonObject data = QJsonObject::fromVariantMap(song);
    if (number(data.value("id")) <= 0) return;
    if (m_roomId && !canControl()) { setError(QStringLiteral("没有房间播放控制权")); return; }
    m_songCache.insert(number(data.value("id")), data);
    fetchCover(data);
    if (m_roomId) {
        if (canControl()) {
            QJsonArray ids = m_roomState.value("songIds").toArray();
            if (!ids.contains(data.value("id"))) {
                ids.append(data.value("id"));
                sendSocket("PLAYLIST_UPDATE", {{"songIds", ids}});
            }
            sendSocket("PLAY", {{"songId", data.value("id")}, {"positionMs", 0}});
        }
    } else {
        if (!m_localQueue.contains(data.value("id"))) {
            m_localQueue.append(data.value("id")); emit localQueueChanged();
        }
        m_player.playSong(data);
    }
}
void AppController::togglePlayback() {
    if (m_roomId) {
        if (!canControl()) return;
        sendSocket(m_player.playing() ? "PAUSE" : "PLAY", {{"songId", m_player.song().value("id").toLongLong()},
                                                          {"positionMs", m_player.position()}});
    } else if (m_player.playing()) m_player.pause(); else m_player.resume();
}
void AppController::seek(qint64 positionMs) {
    if (m_roomId) { if (canControl()) sendSocket("SEEK", {{"positionMs", positionMs}}); }
    else m_player.seek(positionMs);
}
void AppController::nextSong() {
    if (m_roomId) { if (canControl()) sendSocket("NEXT", {}); return; }
    const qint64 current = m_player.song().value("id").toLongLong();
    if (!current && !m_localQueue.isEmpty()) { playLocalById(number(m_localQueue.first())); return; }
    for (int index = 0; index < m_localQueue.size(); ++index) {
        if (number(m_localQueue.at(index)) == current) {
            if (index + 1 < m_localQueue.size()) playLocalById(number(m_localQueue.at(index + 1)));
            return;
        }
    }
}
void AppController::previousSong() {
    const qint64 current = m_player.song().value("id").toLongLong();
    if (m_roomId) {
        if (!canControl()) return;
        const QJsonArray ids = m_roomState.value("songIds").toArray();
        for (int index = 1; index < ids.size(); ++index) {
            if (number(ids.at(index)) == current) {
                sendSocket("PLAY", {{"songId", ids.at(index - 1)}, {"positionMs", 0}}); return;
            }
        }
        return;
    }
    for (int index = 1; index < m_localQueue.size(); ++index) {
        if (number(m_localQueue.at(index)) == current) {
            playLocalById(number(m_localQueue.at(index - 1))); return;
        }
    }
}
void AppController::playLocalById(qint64 id) {
    const QJsonObject song = findSong(id);
    if (!song.isEmpty()) { m_player.playSong(song); return; }
    m_api.request("GET", "api/songs/" + QString::number(id), {}, [this](const QJsonValue &value) {
        const QJsonObject song = object(value);
        m_songCache.insert(number(song.value("id")), song); emit songsChanged();
        fetchCover(song); m_player.playSong(song);
    });
}
void AppController::addToQueue(qint64 songId) {
    if (m_roomId && canControl()) addToRoomQueue(songId);
    else addToLocalQueue(songId);
}
void AppController::addToLocalQueue(qint64 songId) {
    if (songId <= 0) return;
    if (!m_localQueue.contains(songId)) {
        m_localQueue.append(songId); emit localQueueChanged();
        emit infoMessage(QStringLiteral("已加入本地播放队列"));
    } else emit infoMessage(QStringLiteral("歌曲已在本地播放队列中"));
}
void AppController::addToRoomQueue(qint64 songId) {
    if (songId <= 0) return;
    if (!m_roomId || !canControl()) { setError(QStringLiteral("需要加入房间并获得播放控制权")); return; }
    QJsonArray ids = m_roomState.value("songIds").toArray(); ids.append(songId);
    if (m_roomState.value("songIds").toArray().contains(songId)) return;
    publishQueue(ids);
}
void AppController::playNext(const QVariantMap &song) {
    const QJsonObject data = QJsonObject::fromVariantMap(song);
    const qint64 songId = number(data.value("id"));
    if (songId <= 0) return;
    if (m_roomId && !canControl()) { setError(QStringLiteral("没有房间播放控制权")); return; }
    const qint64 current = m_roomId ? number(m_roomState.value("playback").toObject().value("currentSongId"))
                                  : m_player.song().value("id").toLongLong();
    if (current == songId) { emit infoMessage(QStringLiteral("这首歌曲正在播放")); return; }
    m_songCache.insert(songId, data); fetchCover(data);
    QJsonArray ids = m_roomId ? m_roomState.value("songIds").toArray() : m_localQueue;
    for (int i = ids.size() - 1; i >= 0; --i) if (number(ids.at(i)) == songId) ids.removeAt(i);
    int insertion = 0;
    for (int i = 0; i < ids.size(); ++i) if (number(ids.at(i)) == current) { insertion = i + 1; break; }
    ids.insert(insertion, songId);
    if (m_roomId) publishQueue(ids);
    else { m_localQueue = ids; emit localQueueChanged(); emit infoMessage(QStringLiteral("已设为下一首播放")); }
}
void AppController::publishQueue(const QJsonArray &ids) {
    if (m_roomId && canControl()) sendSocket("PLAYLIST_UPDATE", {{"songIds", ids}});
}
void AppController::moveQueueSong(int index, int direction) {
    if (m_roomId && !canControl()) return;
    QJsonArray ids = m_roomId ? m_roomState.value("songIds").toArray() : m_localQueue;
    const int target = index + direction;
    if (index < 0 || index >= ids.size() || target < 0 || target >= ids.size()) return;
    const QJsonValue song = ids.at(index);
    ids.replace(index, ids.at(target)); ids.replace(target, song);
    if (m_roomId) publishQueue(ids);
    else { m_localQueue = ids; emit localQueueChanged(); }
}
void AppController::removeQueueSong(int index) {
    if (m_roomId && !canControl()) return;
    QJsonArray ids = m_roomId ? m_roomState.value("songIds").toArray() : m_localQueue;
    if (index < 0 || index >= ids.size()) return;
    ids.removeAt(index);
    if (m_roomId) publishQueue(ids);
    else { m_localQueue = ids; emit localQueueChanged(); }
}
void AppController::refreshPartners() {
    m_api.request("GET", "api/partners", {}, [this](const QJsonValue &value) { m_partners = array(value); emit partnersChanged(); });
}
void AppController::searchUsers(const QString &keyword) {
    m_userQuery = keyword.trimmed();
    m_userPage = 0; m_userTotal = m_userQuery.isEmpty() ? 0 : 1;
    m_userResults = {}; m_usersLoading = false; ++m_userSearchGeneration; emit userResultsChanged();
    loadMoreUsers();
}
void AppController::loadMoreUsers() {
    if (m_userQuery.isEmpty() || m_usersLoading || !hasMoreUsers()) return;
    m_usersLoading = true;
    const quint64 generation = m_userSearchGeneration;
    const int page = m_userPage + 1;
    m_api.request("GET", "api/users/search?keyword=" + segment(m_userQuery) + "&pageNum=" + QString::number(page) + "&pageSize=40", {},
                  [this, generation, page](const QJsonValue &value) {
        if (generation != m_userSearchGeneration) return;
        const QJsonObject result = object(value);
        m_userTotal = static_cast<int>(number(result.value("total")));
        for (const QJsonValue &user : result.value("list").toArray()) m_userResults.append(user);
        m_userPage = page; m_usersLoading = false; emit userResultsChanged();
    }, [this, generation] { if (generation == m_userSearchGeneration) m_usersLoading = false; });
}
void AppController::addPartner(qint64 id) {
    m_api.request("POST", "api/partners/" + QString::number(id), {},
                  [this](const QJsonValue &) { refreshPartners(); });
}
void AppController::removePartner(qint64 id) {
    m_api.request("DELETE", "api/partners/" + QString::number(id), {},
                  [this](const QJsonValue &) { refreshPartners(); });
}
void AppController::invitePartner(qint64 id) {
    if (!m_roomId) { setError(QStringLiteral("请先进入房间")); return; }
    m_api.request("POST", "api/rooms/" + QString::number(m_roomId) + "/invite", {{"partnerId", id}},
                  [this](const QJsonValue &) { emit infoMessage(QStringLiteral("邀请已发送")); });
}
void AppController::refreshInvites() {
    if (!authenticated()) return;
    m_api.request("GET", "api/invites", {}, [this](const QJsonValue &value) {
        m_invites = array(value); emit invitesChanged();
    });
}
void AppController::respondInvite(const QString &id, bool accept) {
    m_api.request("POST", "api/invites/" + segment(id) + (accept ? "/accept" : "/decline"), {},
                  [this, accept](const QJsonValue &value) {
        refreshInvites();
        if (accept) {
            const QJsonObject result = object(value);
            const qint64 roomId = number(result.value("room").toObject().value("id"));
            if (roomId) { enterRoom(roomId); setRoomState(result); }
            else refreshRooms();
        }
    });
}
void AppController::refreshPlaylists() {
    if (!authenticated()) return;
    const quint64 generation = ++m_playlistsGeneration;
    m_api.request("GET", "api/playlists", {}, [this, generation](const QJsonValue &value) {
        if (generation != m_playlistsGeneration) return;
        m_playlists = array(value); emit playlistsChanged();
    });
}
void AppController::createPlaylist(const QString &name) {
    m_api.request("POST", "api/playlists", {{"name", name.trimmed()}, {"description", ""}},
                  [this](const QJsonValue &) { refreshPlaylists(); });
}
void AppController::createPlaylistWithSong(const QString &name, qint64 songId) {
    const QString normalized = name.trimmed();
    if (!authenticated() || normalized.isEmpty() || songId <= 0 || m_playlistCreationBusy) return;
    m_playlistCreationBusy = true; emit playlistCreationBusyChanged();
    const auto finish = [this, songId](bool added) {
        m_playlistCreationBusy = false; emit playlistCreationBusyChanged();
        emit playlistCreationFinished(songId, added);
    };
    m_api.request("POST", "api/playlists", {{"name", normalized}, {"description", ""}},
                  [this, songId, finish](const QJsonValue &value) {
        const qint64 id = number(object(value).value("id"));
        if (id <= 0) { setError(QStringLiteral("服务器未返回新歌单编号")); finish(false); return; }
        m_api.request("POST", "api/playlists/" + QString::number(id) + "/songs", {{"songIds", QJsonArray{songId}}},
                      [this, finish](const QJsonValue &) {
            refreshPlaylists(); finish(true); emit infoMessage(QStringLiteral("已创建歌单并添加歌曲"));
        }, [this, finish] {
            refreshPlaylists(); finish(false);
            emit infoMessage(QStringLiteral("歌单已创建，歌曲添加失败，可在歌单页重试"));
        });
    }, [finish] { finish(false); });
}
qint64 AppController::favoritePlaylistId() const {
    for (const auto &entry : m_playlists) {
        const QJsonObject playlist = entry.toObject();
        if (playlist.value("name").toString() == QStringLiteral("我喜欢")) return number(playlist.value("id"));
    }
    return 0;
}
QVariantList AppController::favoriteSongIds() const {
    QVariantList ids;
    const qint64 favoriteId = favoritePlaylistId();
    for (const auto &entry : m_playlists) {
        const QJsonObject playlist = entry.toObject();
        if (number(playlist.value("id")) != favoriteId) continue;
        for (const auto &song : playlist.value("songs").toArray()) ids.append(number(song.toObject().value("id")));
        break;
    }
    return ids;
}
void AppController::toggleFavorite(qint64 songId) {
    if (!authenticated() || songId <= 0 || m_favoriteBusy) return;
    m_favoriteBusy = true; emit playlistsChanged();
    const auto failure = [this] { m_favoriteBusy = false; emit playlistsChanged(); };
    const auto update = [this, songId, failure](qint64 playlistId, bool remove) {
        const QString path = "api/playlists/" + QString::number(playlistId) + "/songs" + (remove ? "/" + QString::number(songId) : "");
        m_api.request(remove ? "DELETE" : "POST", path, remove ? QJsonObject{} : QJsonObject{{"songIds", QJsonArray{songId}}},
                      [this, remove, failure](const QJsonValue &) {
            const quint64 generation = ++m_playlistsGeneration;
            m_api.request("GET", "api/playlists", {}, [this, remove, generation](const QJsonValue &value) {
                if (generation != m_playlistsGeneration) { m_favoriteBusy = false; emit playlistsChanged(); return; }
                m_playlists = array(value); m_favoriteBusy = false; emit playlistsChanged();
                emit infoMessage(remove ? QStringLiteral("已取消喜欢") : QStringLiteral("已添加到我喜欢"));
            }, failure);
        }, failure);
    };
    // 首次加载或刷新尚未完成时也先确认服务端歌单，避免把缓存为空误判为尚未创建。
    const quint64 generation = ++m_playlistsGeneration;
    m_api.request("GET", "api/playlists", {}, [this, songId, update, failure, generation](const QJsonValue &value) {
        if (generation != m_playlistsGeneration) { failure(); return; }
        m_playlists = array(value); emit playlistsChanged();
        const qint64 playlistId = favoritePlaylistId();
        if (playlistId > 0) { update(playlistId, favoriteSongIds().contains(QVariant::fromValue(songId))); return; }
        m_api.request("POST", "api/playlists", {{"name", QStringLiteral("我喜欢")}, {"description", ""}},
                      [this, update, failure](const QJsonValue &created) {
            const QJsonObject playlist = object(created);
            const qint64 id = number(playlist.value("id"));
            if (id <= 0) { setError(QStringLiteral("服务器未返回歌单编号")); failure(); return; }
            m_playlists.append(playlist); emit playlistsChanged(); update(id, false);
        }, failure);
    }, failure);
}
void AppController::updatePlaylist(qint64 playlistId, const QString &name,
                                   const QString &description, const QString &coverUrl, bool server) {
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) { setError(QStringLiteral("歌单名称不能为空")); return; }
    if (server && !canManageServerPlaylist()) {
        setError(QStringLiteral("当前账号没有服务器歌单管理权限"));
        return;
    }
    const QString path = (server ? "api/server-playlists/" : "api/playlists/")
                         + QString::number(playlistId);
    m_api.request("PUT", path, {{"name", normalizedName}, {"description", description}, {"coverUrl", coverUrl}},
                  [this, server](const QJsonValue &) {
        if (server) refreshServerPlaylists(); else refreshPlaylists();
        emit infoMessage(QStringLiteral("歌单名称已保存"));
    });
}
void AppController::addSongToPlaylist(qint64 playlistId, qint64 songId) {
    addSongsToPlaylist(playlistId, {QVariant::fromValue(songId)}, false);
}
void AppController::addSongsToPlaylist(qint64 playlistId, const QVariantList &songIds, bool server) {
    if (songIds.isEmpty()) return;
    if (server && !canManageServerPlaylist()) {
        setError(QStringLiteral("当前账号没有服务器歌单管理权限"));
        return;
    }
    QJsonArray ids;
    for (const QVariant &songId : songIds) ids.append(songId.toLongLong());
    const QString path = (server ? "api/server-playlists/" : "api/playlists/")
                         + QString::number(playlistId) + "/songs";
    m_api.request("POST", path, {{"songIds", ids}}, [this, server](const QJsonValue &value) {
        if (server) refreshServerPlaylists(); else refreshPlaylists();
        const QJsonObject result = object(value);
        emit infoMessage(QStringLiteral("成功加入 %1 首（%2 首已存在）")
                         .arg(number(result.value("added")))
                         .arg(number(result.value("duplicated"))));
    });
}
void AppController::refreshServerPlaylists() {
    if (!authenticated()) return;
    const quint64 generation = ++m_serverPlaylistsGeneration;
    m_api.request("GET", "api/server-playlists", {}, [this, generation](const QJsonValue &value) {
        if (generation != m_serverPlaylistsGeneration) return;
        m_serverPlaylists = array(value); emit serverPlaylistsChanged();
    });
}
void AppController::createServerPlaylist(const QString &name) {
    if (!canManageServerPlaylist() || name.trimmed().isEmpty()) return;
    m_api.request("POST", "api/server-playlists", {{"name", name.trimmed()}, {"description", ""}},
                  [this](const QJsonValue &) { refreshServerPlaylists(); });
}
void AppController::addSongToServerPlaylist(qint64 playlistId, qint64 songId) {
    addSongsToPlaylist(playlistId, {QVariant::fromValue(songId)}, true);
}
void AppController::deletePlaylist(qint64 playlistId, bool server) {
    if (server && !canManageServerPlaylist()) return;
    m_api.request("DELETE", (server ? "api/server-playlists/" : "api/playlists/") + QString::number(playlistId), {},
                  [this, server](const QJsonValue &) { if (server) refreshServerPlaylists(); else refreshPlaylists(); });
}
void AppController::removeSongFromPlaylist(qint64 playlistId, qint64 songId, bool server) {
    if (server && !canManageServerPlaylist()) return;
    m_api.request("DELETE", (server ? "api/server-playlists/" : "api/playlists/") + QString::number(playlistId)
                  + "/songs/" + QString::number(songId), {}, [this, server](const QJsonValue &) {
        if (server) refreshServerPlaylists(); else refreshPlaylists();
    });
}
void AppController::reorderPlaylist(qint64 playlistId, const QVariantList &songIds, bool server) {
    if (server && !canManageServerPlaylist()) return;
    QJsonArray ids;
    for (const QVariant &id : songIds) ids.append(id.toLongLong());
    m_api.request("PUT", (server ? "api/server-playlists/" : "api/playlists/") + QString::number(playlistId)
                  + "/order", {{"songIds", ids}}, [this, server](const QJsonValue &) {
        if (server) refreshServerPlaylists(); else refreshPlaylists();
    });
}
void AppController::loadLyrics(qint64 songId) {
    const quint64 generation = ++m_lyricsGeneration;
    m_lyrics = {};
    emit lyricsChanged();
    if (songId <= 0 || !authenticated()) return;
    m_api.request("GET", "api/songs/" + QString::number(songId) + "/lyrics", {},
                  [this, songId, generation](const QJsonValue &value) {
        if (generation != m_lyricsGeneration || m_player.song().value("id").toLongLong() != songId) return;
        const QJsonObject result = object(value);
        if (number(result.value("songId")) != songId) return;
        m_lyrics = result;
        emit lyricsChanged();
    });
}
void AppController::uploadSong(const QString &audioUrl, const QString &title, const QString &artist,
                               qint64 albumId, const QString &newAlbumName, const QString &lyrics, const QString &lyricsUrl,
                               const QString &coverUrl) {
    if (!canUpload()) { setError(QStringLiteral("当前账号没有上传权限")); return; }
    auto *parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    appendTextPart(parts, "title", title);
    appendTextPart(parts, "artist", artist);
    if (albumId > 0) appendTextPart(parts, "albumId", QString::number(albumId));
    else if (!newAlbumName.trimmed().isEmpty()) appendTextPart(parts, "newAlbumName", newAlbumName.trimmed());
    if (lyricsUrl.isEmpty()) appendTextPart(parts, "lyricsText", lyrics);
    if (!appendFilePart(parts, "file", audioUrl) || !appendFilePart(parts, "lyricsFile", lyricsUrl) || !appendFilePart(parts, "albumCoverFile", coverUrl)) {
        delete parts; setError(QStringLiteral("上传文件无法读取")); return;
    }
    m_api.upload("api/songs/upload", parts, [this](const QJsonValue &) {
        emit infoMessage(QStringLiteral("歌曲上传完成")); searchSongs({}); refreshAlbums();
    });
}
void AppController::updateSong(qint64 id, const QString &audioUrl, const QString &title,
                               const QString &artist, qint64 albumId, const QString &albumName,
                               const QString &newAlbumName, const QString &lyrics,
                               const QString &lyricsUrl, const QString &coverUrl) {
    if (!admin()) { setError(QStringLiteral("只有管理员可以编辑歌曲")); return; }
    auto *parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    appendTextPart(parts, "title", title);
    appendTextPart(parts, "artist", artist);
    appendTextPart(parts, "albumId", QString::number(albumId));
    if (!newAlbumName.trimmed().isEmpty()) appendTextPart(parts, "newAlbumName", newAlbumName.trimmed());
    else if (!albumName.isEmpty()) appendTextPart(parts, "albumName", albumName);
    if (lyricsUrl.isEmpty()) appendTextPart(parts, "lyricsText", lyrics);
    if (!appendFilePart(parts, "file", audioUrl) || !appendFilePart(parts, "lyricsFile", lyricsUrl) || !appendFilePart(parts, "albumCoverFile", coverUrl)) {
        delete parts; setError(QStringLiteral("选择的文件无法读取")); return;
    }
    const bool albumCoverChanged = !coverUrl.isEmpty();
    m_api.putUpload("api/songs/" + QString::number(id), parts, [this, id, albumCoverChanged](const QJsonValue &value) {
        invalidateCoverCache(albumCoverChanged ? 0 : id);
        const QJsonObject updatedSong = object(value);
        if (!updatedSong.isEmpty()) m_songCache.insert(id, updatedSong);
        emit infoMessage(QStringLiteral("歌曲已更新")); searchSongs(m_songQuery); refreshAlbums(); emit songSaved();
    });
}
void AppController::deleteSong(qint64 id) {
    if (!admin()) { setError(QStringLiteral("只有管理员可以删除歌曲")); return; }
    m_api.request("DELETE", "api/songs/" + QString::number(id), {}, [this, id](const QJsonValue &) {
        invalidateCoverCache(id);
        m_songCache.remove(id);
        emit infoMessage(QStringLiteral("歌曲已删除")); searchSongs(m_songQuery); emit songDeleted();
    });
}
QVariantList AppController::guessTrackMetadata(const QString &audioUrl) const {
    QString baseName = QFileInfo(QUrl(audioUrl).toLocalFile()).completeBaseName().trimmed();
    baseName.remove(QRegularExpression(QStringLiteral("^\\s*\\d{1,3}[. _-]+")));
    baseName.replace(QRegularExpression(QStringLiteral("\\s+")), " ");
    if (baseName.isEmpty()) return {};
    QVariantList candidates;
    const QRegularExpression split(QStringLiteral("^(.+?)\\s*(?:[-—–_]|\\bby\\b)\\s*(.+)$"), QRegularExpression::CaseInsensitiveOption);
    const auto match = split.match(baseName);
    if (match.hasMatch() && !match.captured(1).trimmed().isEmpty() && !match.captured(2).trimmed().isEmpty()) {
        const QString left = match.captured(1).trimmed();
        const QString right = match.captured(2).trimmed();
        candidates.append(QVariantMap{{"label", QStringLiteral("歌名在前：%1 · %2").arg(left, right)}, {"title", left}, {"artist", right}});
        candidates.append(QVariantMap{{"label", QStringLiteral("歌手在前：%1 · %2").arg(left, right)}, {"title", right}, {"artist", left}});
    } else {
        candidates.append(QVariantMap{{"label", QStringLiteral("使用文件名作为歌名：%1").arg(baseName)}, {"title", baseName}, {"artist", QString()} });
    }
    return candidates;
}
void AppController::refreshAdminUsers() {
    if (!admin()) return;
    m_adminPage = 0; m_adminTotal = 1; m_adminUsers = {}; m_adminLoading = false;
    ++m_adminSearchGeneration; emit adminUsersChanged(); loadMoreAdminUsers();
}
void AppController::loadMoreAdminUsers() {
    if (!admin() || m_adminLoading || !hasMoreAdminUsers()) return;
    m_adminLoading = true;
    const quint64 generation = m_adminSearchGeneration;
    const int page = m_adminPage + 1;
    m_api.request("GET", "api/admin/users?pageNum=" + QString::number(page) + "&pageSize=40", {},
                  [this, generation, page](const QJsonValue &value) {
        if (generation != m_adminSearchGeneration) return;
        const QJsonObject result = object(value);
        m_adminTotal = static_cast<int>(number(result.value("total")));
        for (const QJsonValue &user : result.value("list").toArray()) m_adminUsers.append(user);
        m_adminPage = page; m_adminLoading = false; emit adminUsersChanged();
    }, [this, generation] { if (generation == m_adminSearchGeneration) m_adminLoading = false; });
}
void AppController::updatePermissions(qint64 id, bool upload, bool serverPlaylist) {
    if (!admin()) return;
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/permissions",
                  {{"canUpload", upload}, {"canManageServerPlaylist", serverPlaylist}},
                  [this](const QJsonValue &) { refreshAdminUsers(); });
}
void AppController::updateRole(qint64 id, const QString &role) {
    if (!admin() || (role != "USER" && role != "ADMIN")) return;
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/role", {{"role", role}},
                  [this](const QJsonValue &) { refreshAdminUsers(); });
}
void AppController::resetPassword(qint64 id, const QString &password) {
    if (!admin() || password.size() < 8) { setError(QStringLiteral("新密码至少 8 位")); return; }
    m_api.request("PUT", "api/admin/users/" + QString::number(id) + "/password", {{"newPassword", password}},
                  [this](const QJsonValue &) { emit infoMessage(QStringLiteral("密码已重置")); });
}
void AppController::kickUser(qint64 id) {
    if (!admin()) return;
    m_api.request("POST", "api/admin/users/" + QString::number(id) + "/kick", {},
                  [this](const QJsonValue &) { refreshAdminUsers(); emit infoMessage(QStringLiteral("已强制用户下线")); });
}
void AppController::connectSocket() {
    if (!authenticated() || m_socket.state() == QAbstractSocket::ConnectedState ||
        m_socket.state() == QAbstractSocket::ConnectingState) return;
    QUrl url = m_api.baseUrl().resolved(QUrl("ws"));
    url.setScheme(url.scheme() == "https" ? "wss" : "ws");
    QUrlQuery query; query.addQueryItem("token", m_api.token()); url.setQuery(query);
    m_socket.open(url);
}
void AppController::sendSocket(const QString &type, const QJsonObject &data) {
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        setError(QStringLiteral("实时连接尚未建立")); return;
    }
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(QJsonObject{{"type", type}, {"data", data}}).toJson(QJsonDocument::Compact)));
}
void AppController::applyPlayback(const QJsonObject &playback) {
    if (!m_roomId) return;
    const qint64 songId = number(playback.value("currentSongId"));
    if (!songId) { m_player.stop(); return; }
    const QJsonObject song = findSong(songId);
    if (song.isEmpty()) {
        const qint64 roomId = m_roomId;
        m_api.request("GET", "api/songs/" + QString::number(songId), {}, [this, playback, roomId](const QJsonValue &value) {
            if (m_roomId != roomId || m_roomState.value("playback").toObject() != playback) return;
            const QJsonObject song = value.toObject();
            m_songCache.insert(number(song.value("id")), song); emit songsChanged();
            fetchCover(song); applyPlayback(playback);
        });
        return;
    }
    fetchCover(song);
    qint64 position = number(playback.value("positionMs"));
    const bool playing = playback.value("status").toString() == "playing";
    if (playing) position += qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() + m_clockOffsetMs
                                           - number(playback.value("serverTimestamp")));
    if (m_player.song().value("id").toLongLong() != songId) m_player.playSong(song, position, playing);
    else {
        if (qAbs(m_player.position() - position) > 500) m_player.seek(position);
        if (playing && !m_player.playing()) m_player.resume();
        else if (!playing && m_player.playing()) m_player.pause();
    }
}
void AppController::handleSocketMessage(const QString &message) {
    const QJsonObject envelope = QJsonDocument::fromJson(message.toUtf8()).object();
    const QString type = envelope.value("type").toString();
    const QJsonObject data = envelope.value("data").toObject();
    if (type == "PONG") {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 sent = number(data.value("echo"));
        const qint64 server = number(data.value("serverTime"));
        if (sent > 0 && server > 0 && now >= sent && now - sent < 2000) {
            const qint64 sample = server - (sent + now) / 2;
            m_clockOffsetMs = m_clockSynced ? (m_clockOffsetMs * 3 + sample) / 4 : sample;
            m_clockSynced = true;
        }
        return;
    }
    if (type == "INVITE") {
        const QString id = data.value("inviteId").toString();
        bool exists = false;
        for (const QJsonValue &entry : m_invites) if (entry.toObject().value("inviteId").toString() == id) exists = true;
        if (!exists) { m_invites.prepend(data); emit invitesChanged(); }
        emit invitationArrived(QStringLiteral("一起听邀请"),
                               data.value("from").toObject().value("nickname").toString() +
                               QStringLiteral(" 邀请你加入「") + data.value("roomName").toString() + QStringLiteral("」"));
        return;
    }
    if (type == "PLAY" || type == "PAUSE" || type == "SEEK" || type == "NEXT" || type == "SYNC") {
        if (!m_roomId) return;
        m_roomState.insert("playback", data); emit roomStateChanged();
        applyPlayback(data); return;
    }
    if (type == "PLAYLIST_UPDATE" || type == "MEMBER_JOIN" || type == "MEMBER_LEAVE" || type == "CONTROL_UPDATE") {
        refreshRoomState(); return;
    }
    if (type == "PARTNER_STATUS") refreshPartners();
    if (type == "ROOM_DELETED") {
        if (number(data.value("roomId")) == m_roomId && m_roomId != 0) {
            const bool owner = roomOwner();
            exitRoom();
            if (!owner) emit infoMessage(QStringLiteral("房间已被创建者解散"));
        }
        refreshRooms(); return;
    }
    if (type == "ERROR") setError(data.value("message").toString());
    if (type == "KICKED") { clearSession(); setError(QStringLiteral("账号已被管理员强制下线")); }
}
