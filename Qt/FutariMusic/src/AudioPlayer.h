#pragma once

#include <QAudioOutput>
#include <QJsonObject>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QObject>
#include <QStringList>

class ApiClient;
class QNetworkReply;

// 音频在后台下载并校验内容哈希后播放，UI 只观察属性。
class AudioPlayer final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap song READ song NOTIFY songChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
public:
    explicit AudioPlayer(ApiClient *api, QObject *parent = nullptr);
    QVariantMap song() const { return m_song.toVariantMap(); }
    qint64 position() const { return m_player.position(); }
    qint64 duration() const { return m_player.duration(); }
    bool playing() const { return m_player.playbackState() == QMediaPlayer::PlayingState; }
    bool loading() const { return m_loading; }
    qreal volume() const { return m_output.volume(); }
    void setVolume(qreal volume);
    QString cacheDirectory() const { return m_cacheDirectory; }
    bool cacheEnabled() const { return m_cacheEnabled; }
    void setCacheDirectory(const QString &directory);
    void setCacheEnabled(bool enabled);
    QStringList protectedCachePaths() const;
    void playSong(const QJsonObject &song, qint64 positionMs = 0, bool autoPlay = true);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void playLocalFile(const QString &path, const QString &title, const QString &artist);
    Q_INVOKABLE void stop();
signals:
    void songChanged();
    void positionChanged();
    void durationChanged();
    void playingChanged();
    void loadingChanged();
    void volumeChanged();
    void errorOccurred(const QString &message);
    void reachedEnd();
private:
    void loadFile(const QString &path, qint64 positionMs, bool autoPlay);
    ApiClient *m_api;
    QMediaPlayer m_player;
    QAudioOutput m_output;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_activeReply;
    QJsonObject m_song;
    QString m_cacheDirectory;
    QString m_activePlaybackPath;
    QString m_activePartialPath;
    QString m_ephemeralPath;
    bool m_cacheEnabled = true;
    bool m_loading = false;
    qint64 m_pendingPosition = 0;
    bool m_pendingPlay = false;
    quint64 m_generation = 0;
};
