#pragma once

#include "ApiClient.h"
#include "AudioMetadataReader.h"
#include "AudioPlayer.h"
#include "TransferManager.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QHash>
#include <QSet>
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QWebSocket>

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY sessionChanged)
    Q_PROPERTY(bool autoLogin READ autoLogin WRITE setAutoLogin NOTIFY autoLoginChanged)
    Q_PROPERTY(QString username READ username NOTIFY sessionChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QVariantList songs READ songs NOTIFY songsChanged)
    Q_PROPERTY(QVariantList albums READ albums NOTIFY albumsChanged)
    Q_PROPERTY(QVariantList albumMatches READ albumMatches NOTIFY albumMatchesChanged)
    Q_PROPERTY(QVariantList transferTasks READ transferTasks NOTIFY transferTasksChanged)
    Q_PROPERTY(QVariantList uploadedSongs READ uploadedSongs NOTIFY uploadedSongsChanged)
    Q_PROPERTY(bool hasMoreUploadedSongs READ hasMoreUploadedSongs NOTIFY uploadedSongsChanged)
    Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory NOTIFY settingsChanged)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString cacheDirectory READ cacheDirectory WRITE setCacheDirectory NOTIFY settingsChanged)
    Q_PROPERTY(qint64 cacheUsageBytes READ cacheUsageBytes NOTIFY settingsChanged)
    Q_PROPERTY(TransferManager* transferManager READ transferManager CONSTANT)
    Q_PROPERTY(bool hasMoreSongs READ hasMoreSongs NOTIFY songsChanged)
    Q_PROPERTY(QVariantList rooms READ rooms NOTIFY roomsChanged)
    Q_PROPERTY(bool hasMoreRooms READ hasMoreRooms NOTIFY roomsChanged)
    Q_PROPERTY(QVariantList partners READ partners NOTIFY partnersChanged)
    Q_PROPERTY(QVariantList userResults READ userResults NOTIFY userResultsChanged)
    Q_PROPERTY(bool hasMoreUsers READ hasMoreUsers NOTIFY userResultsChanged)
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY playlistsChanged)
    Q_PROPERTY(QVariantList serverPlaylists READ serverPlaylists NOTIFY serverPlaylistsChanged)
    Q_PROPERTY(QVariantList invites READ invites NOTIFY invitesChanged)
    Q_PROPERTY(QVariantMap roomState READ roomState NOTIFY roomStateChanged)
    Q_PROPERTY(QVariantList localQueue READ localQueue NOTIFY localQueueChanged)
    Q_PROPERTY(QVariantList favoriteSongIds READ favoriteSongIds NOTIFY playlistsChanged)
    Q_PROPERTY(bool favoriteBusy READ favoriteBusy NOTIFY playlistsChanged)
    Q_PROPERTY(bool playlistCreationBusy READ playlistCreationBusy NOTIFY playlistCreationBusyChanged)
    Q_PROPERTY(QVariantMap lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(bool canUpload READ canUpload NOTIFY sessionChanged)
    Q_PROPERTY(bool canManageServerPlaylist READ canManageServerPlaylist NOTIFY sessionChanged)
    Q_PROPERTY(bool admin READ admin NOTIFY sessionChanged)
    Q_PROPERTY(QVariantList adminUsers READ adminUsers NOTIFY adminUsersChanged)
    Q_PROPERTY(bool hasMoreAdminUsers READ hasMoreAdminUsers NOTIFY adminUsersChanged)
    Q_PROPERTY(bool canControl READ canControl NOTIFY roomStateChanged)
    Q_PROPERTY(bool roomOwner READ roomOwner NOTIFY roomStateChanged)
    Q_PROPERTY(bool roomDeletionBusy READ roomDeletionBusy NOTIFY roomDeletionBusyChanged)
    Q_PROPERTY(QVariantMap covers READ covers NOTIFY coversChanged)
    Q_PROPERTY(int unreadInvites READ unreadInvites NOTIFY invitesChanged)
    Q_PROPERTY(AudioPlayer* player READ player CONSTANT)
public:
    explicit AppController(QObject *parent = nullptr);
    bool authenticated() const { return m_authenticated; }
    bool autoLogin() const { return m_autoLogin; }
    void setAutoLogin(bool value);
    QString username() const { return m_username; }
    QString serverUrl() const { return m_api.baseUrl().toString(); }
    void setServerUrl(const QString &url);
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool value);
    QString errorMessage() const { return m_errorMessage; }
    QVariantList songs() const { return m_songs.toVariantList(); }
    QVariantList albums() const { return m_albums.toVariantList(); }
    QVariantList albumMatches() const { return m_albumMatches.toVariantList(); }
    QVariantList transferTasks() const { return m_transfers.tasks(); }
    QVariantList uploadedSongs() const { return m_uploadedSongs.toVariantList(); }
    bool hasMoreUploadedSongs() const { return m_uploadedSongs.size() < m_uploadedSongTotal; }
    QString downloadDirectory() const;
    void setDownloadDirectory(const QString &directory);
    bool cacheEnabled() const { return m_settings.value("cacheEnabled", true).toBool(); }
    void setCacheEnabled(bool enabled);
    QString cacheDirectory() const;
    void setCacheDirectory(const QString &directory);
    qint64 cacheUsageBytes() const;
    TransferManager *transferManager() { return &m_transfers; }
    bool hasMoreSongs() const { return m_songs.size() < m_songTotal; }
    QVariantList rooms() const { return m_rooms.toVariantList(); }
    bool hasMoreRooms() const { return m_rooms.size() < m_roomTotal; }
    QVariantList partners() const { return m_partners.toVariantList(); }
    QVariantList userResults() const { return m_userResults.toVariantList(); }
    bool hasMoreUsers() const { return m_userResults.size() < m_userTotal; }
    QVariantList playlists() const { return m_playlists.toVariantList(); }
    QVariantList serverPlaylists() const { return m_serverPlaylists.toVariantList(); }
    QVariantList invites() const { return m_invites.toVariantList(); }
    QVariantMap roomState() const { return m_roomState.toVariantMap(); }
    QVariantList localQueue() const { return m_localQueue.toVariantList(); }
    QVariantMap lyrics() const { return m_lyrics.toVariantMap(); }
    bool canUpload() const { return admin() || m_canUpload; }
    bool canManageServerPlaylist() const { return admin() || m_canManageServerPlaylist; }
    bool admin() const { return m_role == "ADMIN"; }
    QVariantList adminUsers() const { return m_adminUsers.toVariantList(); }
    bool hasMoreAdminUsers() const { return m_adminUsers.size() < m_adminTotal; }
    bool canControl() const;
    bool roomOwner() const { return m_roomId && m_roomState.value("room").toObject().value("ownerId").toVariant().toLongLong() == m_userId; }
    bool roomDeletionBusy() const { return m_deletingRoomId != 0; }
    QVariantMap covers() const { return m_covers; }
    int unreadInvites() const { return m_invites.size(); }
    AudioPlayer *player() { return &m_player; }

    Q_INVOKABLE void login(const QString &username, const QString &password);
    Q_INVOKABLE void registerAccount(const QString &username, const QString &nickname, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void searchSongs(const QString &keyword);
    Q_INVOKABLE void loadMoreSongs();
    Q_INVOKABLE void refreshAlbums(const QString &keyword = QString());
    Q_INVOKABLE void refreshUploadedSongs();
    Q_INVOKABLE void loadMoreUploadedSongs();
    Q_INVOKABLE QString enqueueUpload(const QVariantMap &metadata);
    Q_INVOKABLE void enqueueDownload(const QVariantMap &song);
    Q_INVOKABLE void openLocalFolder(const QString &path);
    Q_INVOKABLE void clearSongCache();
    Q_INVOKABLE void searchAlbumMatches(const QString &name, const QString &artist);
    Q_INVOKABLE void inspectAudio(const QString &audioUrl, const QString &requestId);
    Q_INVOKABLE QVariantMap findLyricsForAudio(const QString &audioUrl) const;
    Q_INVOKABLE QString readLyricsFile(const QString &lyricsUrl) const;
    Q_INVOKABLE void refreshRooms();
    Q_INVOKABLE void loadMoreRooms();
    Q_INVOKABLE void createRoom(const QString &name);
    Q_INVOKABLE void joinRoom(qint64 id);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE bool canDeleteRoom(qint64 roomId) const;
    Q_INVOKABLE void deleteRoom(qint64 roomId);
    Q_INVOKABLE void refreshRoomState();
    Q_INVOKABLE void restoreCurrentRoom();
    Q_INVOKABLE void setController(qint64 memberId, bool enabled);
    Q_INVOKABLE void playSong(const QVariantMap &song);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void nextSong();
    Q_INVOKABLE void previousSong();
    Q_INVOKABLE void addToQueue(qint64 songId);
    Q_INVOKABLE void addToLocalQueue(qint64 songId);
    Q_INVOKABLE void addToRoomQueue(qint64 songId);
    Q_INVOKABLE void playNext(const QVariantMap &song);
    QVariantList favoriteSongIds() const;
    bool favoriteBusy() const { return m_favoriteBusy; }
    bool playlistCreationBusy() const { return m_playlistCreationBusy; }
    Q_INVOKABLE void toggleFavorite(qint64 songId);
    Q_INVOKABLE void createPlaylistWithSong(const QString &name, qint64 songId);
    Q_INVOKABLE void moveQueueSong(int index, int direction);
    Q_INVOKABLE void removeQueueSong(int index);
    Q_INVOKABLE void refreshPartners();
    Q_INVOKABLE void searchUsers(const QString &keyword);
    Q_INVOKABLE void loadMoreUsers();
    Q_INVOKABLE void addPartner(qint64 id);
    Q_INVOKABLE void removePartner(qint64 id);
    Q_INVOKABLE void invitePartner(qint64 id);
    Q_INVOKABLE void refreshInvites();
    Q_INVOKABLE void respondInvite(const QString &id, bool accept);
    Q_INVOKABLE void refreshPlaylists();
    Q_INVOKABLE void createPlaylist(const QString &name);
    Q_INVOKABLE void updatePlaylist(qint64 playlistId, const QString &name,
                                    const QString &description, const QString &coverUrl, bool server);
    Q_INVOKABLE void addSongsToPlaylist(qint64 playlistId, const QVariantList &songIds, bool server);
    Q_INVOKABLE void addSongToPlaylist(qint64 playlistId, qint64 songId);
    Q_INVOKABLE void refreshServerPlaylists();
    Q_INVOKABLE void createServerPlaylist(const QString &name);
    Q_INVOKABLE void addSongToServerPlaylist(qint64 playlistId, qint64 songId);
    Q_INVOKABLE void deletePlaylist(qint64 playlistId, bool server);
    Q_INVOKABLE void removeSongFromPlaylist(qint64 playlistId, qint64 songId, bool server);
    Q_INVOKABLE void reorderPlaylist(qint64 playlistId, const QVariantList &songIds, bool server);
    Q_INVOKABLE void loadLyrics(qint64 songId);
    Q_INVOKABLE QVariantMap songForId(qint64 songId) const { return findSong(songId).toVariantMap(); }
    Q_INVOKABLE void uploadSong(const QString &audioUrl, const QString &title,
                                const QString &artist, qint64 albumId,
                                const QString &newAlbumName, const QString &lyrics,
                                const QString &lyricsUrl, const QString &coverUrl);
    Q_INVOKABLE void updateSong(qint64 id, const QString &audioUrl, const QString &title,
                                const QString &artist, qint64 albumId,
                                const QString &albumName, const QString &newAlbumName,
                                const QString &lyrics, const QString &lyricsUrl,
                                const QString &coverUrl);
    Q_INVOKABLE void deleteSong(qint64 id);
    Q_INVOKABLE QVariantList guessTrackMetadata(const QString &audioUrl) const;
    Q_INVOKABLE void refreshAdminUsers();
    Q_INVOKABLE void loadMoreAdminUsers();
    Q_INVOKABLE void updatePermissions(qint64 id, bool upload, bool serverPlaylist);
    Q_INVOKABLE void updateRole(qint64 id, const QString &role);
    Q_INVOKABLE void resetPassword(qint64 id, const QString &password);
    Q_INVOKABLE void kickUser(qint64 id);
signals:
    void sessionChanged();
    void autoLoginChanged();
    void serverUrlChanged();
    void darkModeChanged();
    void errorMessageChanged();
    void songsChanged();
    void albumsChanged();
    void albumMatchesChanged();
    void transferTasksChanged();
    void uploadedSongsChanged();
    void settingsChanged();
    void roomsChanged();
    void partnersChanged();
    void userResultsChanged();
    void playlistsChanged();
    void serverPlaylistsChanged();
    void invitesChanged();
    void roomStateChanged();
    void localQueueChanged();
    void lyricsChanged();
    void roomDeletionBusyChanged();
    void coversChanged();
    void adminUsersChanged();
    void invitationArrived(const QString &title, const QString &message);
    void openMessagesRequested();
    void infoMessage(const QString &message);
    void songSaved();
    void songDeleted();
    void audioMetadataReady(const QString &requestId, const QVariantMap &metadata);
    void playlistCreationBusyChanged();
    void playlistCreationFinished(qint64 songId, bool added);
private:
    void connectSocket();
    void handleSocketMessage(const QString &message);
    void exitRoom();
    void sendSocket(const QString &type, const QJsonObject &data);
    void setRoomState(const QJsonObject &state);
    void applyPlayback(const QJsonObject &playback);
    void playLocalById(qint64 id);
    void publishQueue(const QJsonArray &ids);
    void enterRoom(qint64 roomId);
    qint64 favoritePlaylistId() const;
    QJsonObject findSong(qint64 id) const;
    void fetchCovers();
    void fetchCover(const QJsonObject &song);
    void invalidateCoverCache(qint64 songId = 0);
    void pruneCoverCache(const QString &protectedPath);
    void setError(const QString &message);
    void clearSession();
    void resetSession(QNetworkReply *logoutReply);
    static QString segment(const QString &value);
    ApiClient m_api;
    AudioMetadataReader m_metadataReader;
    TransferManager m_transfers;
    AudioPlayer m_player;
    QWebSocket m_socket;
    QTimer m_reconnect;
    QTimer m_heartbeat;
    QSettings m_settings;
    QString m_username;
    QString m_role;
    QString m_errorMessage;
    bool m_darkMode = false;
    bool m_authenticated = false;
    bool m_autoLogin = false;
    bool m_canUpload = false;
    bool m_canManageServerPlaylist = false;
    qint64 m_userId = 0;
    qint64 m_roomId = 0;
    qint64 m_deletingRoomId = 0;
    QJsonArray m_songs, m_albums, m_albumMatches, m_uploadedSongs, m_rooms, m_partners, m_userResults, m_playlists, m_serverPlaylists, m_invites, m_adminUsers;
    QJsonObject m_roomState, m_lyrics;
    quint64 m_lyricsGeneration = 0;
    QJsonArray m_localQueue;
    QJsonObject m_savedLocalSong;
    qint64 m_savedLocalPosition = 0;
    bool m_favoriteBusy = false;
    bool m_playlistCreationBusy = false;
    quint64 m_playlistsGeneration = 0;
    quint64 m_serverPlaylistsGeneration = 0;
    QHash<qint64, QJsonObject> m_songCache;
    QVariantMap m_covers;
    QSet<QString> m_coverLoading;
    quint64 m_coverGeneration = 0;
    QString m_songQuery;
    int m_songPage = 0;
    int m_roomPage = 0;
    int m_songTotal = 0;
    int m_uploadedSongTotal = 0;
    int m_uploadedSongPage = 0;
    quint64 m_uploadedSongsGeneration = 0;
    int m_roomTotal = 0;
    quint64 m_songSearchGeneration = 0;
    quint64 m_albumSearchGeneration = 0;
    quint64 m_roomSearchGeneration = 0;
    bool m_songsLoading = false;
    bool m_uploadedSongsLoading = false;
    bool m_roomsLoading = false;
    QString m_userQuery;
    int m_userPage = 0;
    int m_userTotal = 0;
    int m_adminPage = 0;
    int m_adminTotal = 0;
    bool m_usersLoading = false;
    bool m_adminLoading = false;
    quint64 m_userSearchGeneration = 0;
    quint64 m_adminSearchGeneration = 0;
    qint64 m_clockOffsetMs = 0;
    bool m_clockSynced = false;
};
