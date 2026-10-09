#ifndef FUTARI_APP_CONTROLLER_H
#define FUTARI_APP_CONTROLLER_H

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QWebSocket>

#include "ApiClient.h"
#include "AudioMetadataReader.h"
#include "AudioPlayer.h"
#include "TransferManager.h"
#include "models/SessionState.h"

// QML 兼容门面：公开属性/方法保持稳定，实现按业务分布在 AppController*.cpp。
// 会话状态集中于 SessionState，播放和传输状态分别由对应服务拥有。
class AppController final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AppController)
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
    Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory NOTIFY
                   settingsChanged)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY settingsChanged)
    Q_PROPERTY(
        QString cacheDirectory READ cacheDirectory WRITE setCacheDirectory NOTIFY settingsChanged)
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
    Q_PROPERTY(
        bool playlistCreationBusy READ playlistCreationBusy NOTIFY playlistCreationBusyChanged)
    Q_PROPERTY(QVariantMap lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(bool canUpload READ canUpload NOTIFY sessionChanged)
    Q_PROPERTY(bool canManageServerPlaylist READ canManageServerPlaylist NOTIFY sessionChanged)
    Q_PROPERTY(bool admin READ admin NOTIFY sessionChanged)
    Q_PROPERTY(QVariantList adminUsers READ adminUsers NOTIFY adminUsersChanged)
    Q_PROPERTY(bool hasMoreAdminUsers READ hasMoreAdminUsers NOTIFY adminUsersChanged)
    Q_PROPERTY(bool canControl READ canControl NOTIFY roomStateChanged)
    Q_PROPERTY(bool roomOwner READ roomOwner NOTIFY roomStateChanged)
    Q_PROPERTY(bool roomDeletionBusy READ roomDeletionBusy NOTIFY roomDeletionBusyChanged)
    Q_PROPERTY(bool batchDeletionBusy READ batchDeletionBusy NOTIFY songsChanged)
    Q_PROPERTY(QVariantMap covers READ covers NOTIFY coversChanged)
    Q_PROPERTY(int unreadInvites READ unreadInvites NOTIFY invitesChanged)
    Q_PROPERTY(AudioPlayer* player READ player CONSTANT)
public:
    explicit AppController(QObject* parent = nullptr);
    bool authenticated() const { return m_session.authenticated; }
    bool autoLogin() const { return m_autoLogin; }
    void setAutoLogin(bool value);
    QString username() const { return m_username; }
    QString serverUrl() const { return m_api.baseUrl().toString(); }
    void setServerUrl(const QString& url);
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool value);
    QString errorMessage() const { return m_errorMessage; }
    QVariantList songs() const { return m_session.songs.items.toVariantList(); }
    QVariantList albums() const { return m_session.albums.toVariantList(); }
    QVariantList albumMatches() const { return m_session.albumMatches.toVariantList(); }
    QVariantList transferTasks() const { return m_transfers.tasks(); }
    QVariantList uploadedSongs() const { return m_session.uploadedSongs.items.toVariantList(); }
    bool hasMoreUploadedSongs() const {
        return m_session.uploadedSongs.items.size() < m_session.uploadedSongs.total;
    }
    QString downloadDirectory() const;
    void setDownloadDirectory(const QString& directory);
    bool cacheEnabled() const { return m_settings.value("cacheEnabled", true).toBool(); }
    void setCacheEnabled(bool enabled);
    QString cacheDirectory() const;
    void setCacheDirectory(const QString& directory);
    qint64 cacheUsageBytes() const;
    TransferManager* transferManager() { return &m_transfers; }
    bool hasMoreSongs() const { return m_session.songs.items.size() < m_session.songs.total; }
    QVariantList rooms() const { return m_session.rooms.items.toVariantList(); }
    bool hasMoreRooms() const { return m_session.rooms.items.size() < m_session.rooms.total; }
    QVariantList partners() const { return m_session.partners.toVariantList(); }
    QVariantList userResults() const { return m_session.userResults.items.toVariantList(); }
    bool hasMoreUsers() const {
        return m_session.userResults.items.size() < m_session.userResults.total;
    }
    QVariantList playlists() const { return m_session.playlists.toVariantList(); }
    QVariantList serverPlaylists() const { return m_session.serverPlaylists.toVariantList(); }
    QVariantList invites() const { return m_session.invites.toVariantList(); }
    QVariantMap roomState() const { return m_session.roomState.toVariantMap(); }
    QVariantList localQueue() const { return m_session.localQueue.toVariantList(); }
    QVariantMap lyrics() const { return m_session.lyrics.toVariantMap(); }
    bool canUpload() const { return admin() || m_session.canUpload; }
    bool canManageServerPlaylist() const { return admin() || m_session.canManageServerPlaylist; }
    bool admin() const { return m_session.role == "ADMIN"; }
    QVariantList adminUsers() const { return m_session.adminUsers.items.toVariantList(); }
    bool hasMoreAdminUsers() const {
        return m_session.adminUsers.items.size() < m_session.adminUsers.total;
    }
    bool canControl() const;
    bool roomOwner() const {
        return m_session.roomId && m_session.roomState.value("room")
                                           .toObject()
                                           .value("ownerId")
                                           .toVariant()
                                           .toLongLong() == m_session.userId;
    }
    bool roomDeletionBusy() const { return m_session.deletingRoomId != 0; }
    bool batchDeletionBusy() const { return m_session.deletingSongs; }
    QVariantMap covers() const { return m_session.covers; }
    int unreadInvites() const { return m_session.invites.size(); }
    AudioPlayer* player() { return &m_player; }

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void registerAccount(const QString& username, const QString& nickname,
                                     const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void searchSongs(const QString& keyword);
    Q_INVOKABLE void loadMoreSongs();
    Q_INVOKABLE void refreshAlbums(const QString& keyword = QString());
    Q_INVOKABLE void refreshUploadedSongs();
    Q_INVOKABLE void loadMoreUploadedSongs();
    Q_INVOKABLE QString enqueueUpload(const QVariantMap& metadata);
    Q_INVOKABLE void enqueueDownload(const QVariantMap& song);
    Q_INVOKABLE void openLocalFolder(const QString& path);
    Q_INVOKABLE void clearSongCache();
    Q_INVOKABLE void searchAlbumMatches(const QString& name, const QString& artist);
    Q_INVOKABLE void inspectAudio(const QString& audioUrl, const QString& requestId);
    Q_INVOKABLE QVariantMap findLyricsForAudio(const QString& audioUrl) const;
    Q_INVOKABLE QString readLyricsFile(const QString& lyricsUrl) const;
    Q_INVOKABLE void refreshRooms();
    Q_INVOKABLE void loadMoreRooms();
    Q_INVOKABLE void createRoom(const QString& name);
    Q_INVOKABLE void joinRoom(qint64 id);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void leaveRoomBeforeClose();
    Q_INVOKABLE bool canDeleteRoom(qint64 roomId) const;
    Q_INVOKABLE void deleteRoom(qint64 roomId);
    Q_INVOKABLE void refreshRoomState();
    Q_INVOKABLE void setController(qint64 memberId, bool enabled);
    Q_INVOKABLE void playSong(const QVariantMap& song);
    Q_INVOKABLE void playSongs(const QVariantList& songIds);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void nextSong();
    Q_INVOKABLE void previousSong();
    Q_INVOKABLE void addToQueue(qint64 songId);
    Q_INVOKABLE void addToLocalQueue(qint64 songId);
    Q_INVOKABLE void addToRoomQueue(qint64 songId);
    Q_INVOKABLE void addSongsToQueue(const QVariantList& songIds);
    Q_INVOKABLE void addSongsToLocalQueue(const QVariantList& songIds);
    Q_INVOKABLE void playNext(const QVariantMap& song);
    QVariantList favoriteSongIds() const;
    bool favoriteBusy() const { return m_session.favoriteBusy; }
    bool playlistCreationBusy() const { return m_session.playlistCreationBusy; }
    Q_INVOKABLE void toggleFavorite(qint64 songId);
    Q_INVOKABLE void createPlaylistWithSong(const QString& name, qint64 songId);
    Q_INVOKABLE void createPlaylistWithSongs(const QString& name, const QVariantList& songIds);
    Q_INVOKABLE void moveQueueSong(int index, int direction);
    Q_INVOKABLE void removeQueueSong(int index);
    Q_INVOKABLE void refreshPartners();
    Q_INVOKABLE void searchUsers(const QString& keyword);
    Q_INVOKABLE void loadMoreUsers();
    Q_INVOKABLE void addPartner(qint64 id);
    Q_INVOKABLE void removePartner(qint64 id);
    Q_INVOKABLE void invitePartner(qint64 id);
    Q_INVOKABLE void refreshInvites();
    Q_INVOKABLE void respondInvite(const QString& id, bool accept);
    Q_INVOKABLE void refreshPlaylists();
    Q_INVOKABLE void createPlaylist(const QString& name);
    Q_INVOKABLE void updatePlaylist(qint64 playlistId, const QString& name,
                                    const QString& description, const QString& coverUrl,
                                    bool server);
    Q_INVOKABLE void addSongsToPlaylist(qint64 playlistId, const QVariantList& songIds,
                                        bool server);
    Q_INVOKABLE void addSongToPlaylist(qint64 playlistId, qint64 songId);
    Q_INVOKABLE void refreshServerPlaylists();
    Q_INVOKABLE void createServerPlaylist(const QString& name);
    Q_INVOKABLE void addSongToServerPlaylist(qint64 playlistId, qint64 songId);
    Q_INVOKABLE void deletePlaylist(qint64 playlistId, bool server);
    Q_INVOKABLE void removeSongFromPlaylist(qint64 playlistId, qint64 songId, bool server);
    Q_INVOKABLE void removeSongsFromPlaylist(qint64 playlistId, const QVariantList& songIds,
                                             bool server);
    Q_INVOKABLE void reorderPlaylist(qint64 playlistId, const QVariantList& songIds, bool server);
    Q_INVOKABLE void loadLyrics(qint64 songId);
    Q_INVOKABLE QVariantMap songForId(qint64 songId) const {
        return findSong(songId).toVariantMap();
    }
    Q_INVOKABLE void uploadSong(const QString& audioUrl, const QString& title,
                                const QString& artist, qint64 albumId, const QString& newAlbumName,
                                const QString& lyrics, const QString& lyricsUrl,
                                const QString& coverUrl);
    Q_INVOKABLE void updateSong(qint64 id, const QString& audioUrl, const QString& title,
                                const QString& artist, qint64 albumId, const QString& albumName,
                                const QString& newAlbumName, const QString& lyrics,
                                const QString& lyricsUrl, const QString& coverUrl);
    Q_INVOKABLE void deleteSong(qint64 id);
    Q_INVOKABLE void deleteSongs(const QVariantList& songIds);
    Q_INVOKABLE QVariantList guessTrackMetadata(const QString& audioUrl) const;
    Q_INVOKABLE void refreshAdminUsers();
    Q_INVOKABLE void loadMoreAdminUsers();
    Q_INVOKABLE void updatePermissions(qint64 id, bool upload, bool serverPlaylist);
    Q_INVOKABLE void updateRole(qint64 id, const QString& role);
    Q_INVOKABLE void resetPassword(qint64 id, const QString& password);
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
    void roomLeaveForCloseFinished();
    void localQueueChanged();
    void lyricsChanged();
    void roomDeletionBusyChanged();
    void coversChanged();
    void adminUsersChanged();
    void invitationArrived(const QString& title, const QString& message);
    void openMessagesRequested();
    void infoMessage(const QString& message);
    void songSaved();
    void songDeleted();
    void audioMetadataReady(const QString& requestId, const QVariantMap& metadata);
    void playlistCreationBusyChanged();
    void playlistCreationFinished(qint64 songId, bool added);

private:
    void restorePreferences();
    void setupConnections();
    void setupSocket();
    void restoreSession();
    void applySession(const QJsonObject& data);
    static void resetPage(PageState& state, const QString& keyword);
    void loadPage(PageState& state, const QString& path, std::function<void()> publish);
    void cacheSongs(const QJsonArray& songs);
    void cachePlaylistSongs(const QJsonArray& playlists);
    void selectAlbumMatches(const QJsonArray& rows, const QString& name, const QString& artist);
    enum class SongAddition { Added, Failed };
    void addCreatedPlaylistSong(qint64 playlistId, qint64 songId);
    void addCreatedPlaylistSongs(qint64 playlistId, const QVariantList& songIds);
    void finishPlaylistCreation(qint64 songId, SongAddition result);
    void finishPlaylistCreation(const QVariantList& songIds, SongAddition result);
    enum class FavoriteChange { Add, Remove };
    void finishFavorite();
    void prepareFavoriteUpdate(qint64 songId);
    void updateFavorite(qint64 playlistId, qint64 songId, FavoriteChange change);
    void reloadFavorite(FavoriteChange change);
    void removeNextPlaylistSong(const RequestScope::Token& token);
    void deleteNextSong(const RequestScope::Token& token);
    void notifySessionReset();
    void connectSocket();
    void updateServerClock(const QJsonObject& data);
    void receiveInvite(const QJsonObject& data);
    void handleSocketMessage(const QString& message);
    void exitRoom();
    void sendSocket(const QString& type, const QJsonObject& data);
    void setRoomState(const QJsonObject& state);
    void applyPlayback(const QJsonObject& playback);
    void playLocalById(qint64 id);
    void publishQueue(const QJsonArray& ids);
    void enterRoom(qint64 roomId);
    qint64 favoritePlaylistId() const;
    QJsonObject findSong(qint64 id) const;
    void fetchCovers();
    void fetchCover(const QJsonObject& song);
    void invalidateCoverCache(qint64 songId = 0);
    void pruneCoverCache(const QString& protectedPath);
    void setError(const QString& message);
    void clearSession();
    void leavePreviousRoom();
    void resetSession(QNetworkReply* logoutReply);
    static QString segment(const QString& value);
    ApiClient m_api;
    AudioMetadataReader m_metadataReader;
    TransferManager m_transfers;
    AudioPlayer m_player;
    QWebSocket m_socket;
    QTimer m_reconnect;
    QTimer m_heartbeat;
    QSettings m_settings;
    // 持久偏好与登录页记忆，不属于可清空的账号会话。
    QString m_username;
    QString m_errorMessage;
    bool m_darkMode = false;
    bool m_autoLogin = false;
    SessionState m_session;
};

#endif  // FUTARI_APP_CONTROLLER_H
