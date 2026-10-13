#ifndef FUTARI_AUDIO_PLAYER_H
#define FUTARI_AUDIO_PLAYER_H

#include <QAudioOutput>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QMediaPlayer>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

#include "AudioStreamServer.h"

class ApiClient;

// 播放器负责媒体状态；认证、Range 转发和分片缓存由 AudioStreamServer 封装。
class AudioPlayer final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AudioPlayer)
    Q_PROPERTY(QVariantMap song READ song NOTIFY songChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(qreal bufferProgress READ bufferProgress NOTIFY bufferProgressChanged)
    Q_PROPERTY(QVariantList bufferedRanges READ bufferedRanges NOTIFY bufferedRangesChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
public:
    explicit AudioPlayer(ApiClient *api, QObject *parent = nullptr);
    ~AudioPlayer() override;
    QVariantMap song() const { return m_song.toVariantMap(); }
    qint64 position() const { return m_player.source().isEmpty() ? m_pendingPosition : m_player.position(); }
    qint64 duration() const {
        const qint64 mediaDuration = m_player.duration();
        return mediaDuration > 0 ? mediaDuration : m_song.value("durationMs").toInteger();
    }
    bool playing() const { return m_player.playbackState() == QMediaPlayer::PlayingState; }
    bool loading() const { return m_loading; }
    bool failed() const { return m_failed; }
    qreal bufferProgress() const { return m_bufferProgress; }
    QVariantList bufferedRanges() const;
    qreal volume() const { return m_output.volume(); }
    void setVolume(qreal volume);
    QString cacheDirectory() const { return m_cacheDirectory; }
    bool cacheEnabled() const { return m_cacheEnabled; }
    void setCacheDirectory(const QString &directory);
    void setCacheEnabled(bool enabled);
    QStringList protectedCachePaths() const;
    void playSong(const QJsonObject &song, qint64 positionMs = 0, bool autoPlay = true,
                  bool advanceWhileLoading = false);
    void restoreSong(const QJsonObject &song, qint64 positionMs);
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
    void bufferProgressChanged();
    void bufferedRangesChanged();
    void volumeChanged();
    void errorOccurred(const QString &message);
    void reachedEnd();
private:
    void loadFile(const QString &path, qint64 positionMs, bool autoPlay);
    void setBufferProgress(qreal progress);
    void setLoading(bool loading);
    // 借用控制器的 API 配置；控制器按成员顺序保证其寿命长于播放器。
    ApiClient *m_api;
    QMediaPlayer m_player;
    QAudioOutput m_output;
    AudioStreamServer m_stream;
    QJsonObject m_song;
    QString m_cacheDirectory;
    // 正在播放/下载的路径不可被缓存清理删除；ephemeralPath 在停止后释放。
    QString m_activePlaybackPath;
    QString m_activePartialPath;
    QString m_ephemeralPath;
    bool m_cacheEnabled = true;
    // Initial source loading and streaming stalls share the same UI state.
    bool m_loading = false;
    bool m_playRequested = false;
    bool m_waitingForSeekProgress = false;
    qint64 m_seekPosition = 0;
    qint64 m_lastObservedPosition = 0;
    QElapsedTimer m_progressClock;
    QTimer m_stallTimer;
    // Distinguishes a server-backed stream from a complete local file for cache-range reporting.
    bool m_streamingSourceActive = false;
    // Initial seek/play intent is applied once when Qt reports usable media metadata.
    bool m_initialMediaReady = false;
    // Highest cached byte frontier for existing bindings; the player bar renders exact intervals.
    qreal m_bufferProgress = 0;
    // 媒体加载完成前暂存用户最新 seek/play 意图；LoadedMedia 后统一应用。
    qint64 m_pendingPosition = 0;
    bool m_pendingPlay = false;
    bool m_advanceWhileLoading = false;
    bool m_failed = false;
    QElapsedTimer m_pendingPositionClock;
};

#endif  // FUTARI_AUDIO_PLAYER_H
