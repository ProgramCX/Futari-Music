#include "AudioPlayer.h"
#include "ApiClient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include <algorithm>

namespace {
constexpr qint64 kProgressStepMs = 25;
constexpr qint64 kSeekResumedMs = 100;
constexpr qint64 kStallTimeoutMs = 900;
constexpr qint64 kSeekToleranceMs = 50;

QByteArray hashFile(QFile &file) {
    QCryptographicHash digest(QCryptographicHash::Sha256);
    while (!file.atEnd()) digest.addData(file.read(64 * 1024));
    return digest.result().toHex();
}

void pruneCache(const QString &directory, const QString &protectedPath) {
    constexpr qint64 maxBytes = 512LL * 1024 * 1024;
    QDir dir(directory);
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 total = 0;
    QFileInfoList complete;
    for (const QFileInfo &file : files) {
        if (file.fileName().endsWith(QStringLiteral(".part.ranges.json"))) {
            if (file.lastModified().daysTo(QDateTime::currentDateTime()) > 7)
                QFile::remove(file.absoluteFilePath());
            continue;
        }
        if (file.suffix() == QStringLiteral("part")) {
            if (file.lastModified().daysTo(QDateTime::currentDateTime()) > 7) {
                QFile::remove(file.absoluteFilePath());
                QFile::remove(file.absoluteFilePath() + QStringLiteral(".ranges.json"));
            }
            continue;
        }
        total += file.size();
        complete.append(file);
    }
    std::sort(complete.begin(), complete.end(), [](const QFileInfo &left, const QFileInfo &right) {
        return left.lastModified() < right.lastModified();
    });
    for (const QFileInfo &file : complete) {
        if (total <= maxBytes) break;
        if (file.absoluteFilePath() == protectedPath) continue;
        if (QFile::remove(file.absoluteFilePath())) total -= file.size();
    }
}
}  // namespace

AudioPlayer::AudioPlayer(ApiClient *api, QObject *parent) : QObject(parent), m_api(api) {
    m_cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/music_cache";
    m_player.setAudioOutput(&m_output);
    m_output.setVolume(0.7);
    connect(&m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (position > m_lastObservedPosition + kProgressStepMs) {
            m_lastObservedPosition = position;
            m_progressClock.restart();
            if (m_waitingForSeekProgress && position >= m_seekPosition + kSeekResumedMs &&
                m_player.mediaStatus() != QMediaPlayer::StalledMedia) {
                m_waitingForSeekProgress = false;
                setLoading(false);
            } else if (!m_waitingForSeekProgress && m_initialMediaReady &&
                       m_player.playbackState() == QMediaPlayer::PlayingState &&
                       m_player.mediaStatus() != QMediaPlayer::StalledMedia) {
                setLoading(false);
            }
        }
        if (position < m_lastObservedPosition - kSeekResumedMs) m_lastObservedPosition = position;
        emit positionChanged();
    });
    connect(&m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, [this] {
        m_progressClock.restart();
        if (!m_playRequested && m_initialMediaReady && !m_waitingForSeekProgress) {
            setLoading(false);
        }
        emit playingChanged();
    });
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if ((status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) &&
                    !m_initialMediaReady) {
                    m_initialMediaReady = true;
                    // Room playback aligns once after initial loading, before play().
                    // The guard excludes subsequent buffering; local playback has no running clock.
                    if (m_pendingPlay && m_pendingPositionClock.isValid())
                        m_pendingPosition += m_pendingPositionClock.elapsed();
                    m_pendingPositionClock.invalidate();
                    m_player.setPosition(m_pendingPosition);
                    setLoading(false);
                    if (m_pendingPlay) m_player.play();
                    m_pendingPlay = false;
                } else if (status == QMediaPlayer::StalledMedia && m_streamingSourceActive &&
                           m_playRequested) {
                    setLoading(true);
                } else if (status == QMediaPlayer::EndOfMedia) {
                    m_playRequested = false;
                    m_waitingForSeekProgress = false;
                    setLoading(false);
                    emit reachedEnd();
                }
            });
    connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error error, const QString &description) {
                m_failed = true;
                m_pendingPosition = m_player.position();
                m_waitingForSeekProgress = false;
                m_playRequested = false;
                setLoading(false);
                m_pendingPlay = false;
                m_pendingPositionClock.invalidate();
                qWarning() << "[audio player] playback failed" << "songId=" << m_song.value("id")
                           << "error=" << error << "position=" << m_player.position()
                           << "status=" << m_player.mediaStatus();
                emit errorOccurred(description);
            });
    connect(&m_stream, &AudioStreamServer::bufferedRangesChanged, this, [this] {
        if (m_streamingSourceActive) setBufferProgress(m_stream.bufferProgress());
        emit bufferedRangesChanged();
    });
    connect(&m_stream, &AudioStreamServer::cacheCompleted, this, [this] {
        m_activePartialPath.clear();
        m_activePlaybackPath = m_stream.completedPath();
        pruneCache(QFileInfo(m_stream.completedPath()).absolutePath(), m_stream.completedPath());
    });
    connect(&m_stream, &AudioStreamServer::sourceError, this, [this](const QString &message) {
        if (m_streamingSourceActive) emit errorOccurred(message);
    });
    m_stallTimer.setInterval(250);
    connect(&m_stallTimer, &QTimer::timeout, this, [this] {
        if (m_streamingSourceActive && m_initialMediaReady && m_playRequested &&
            m_progressClock.isValid() && m_progressClock.elapsed() > kStallTimeoutMs)
            setLoading(true);
    });
    m_stallTimer.start();
}

AudioPlayer::~AudioPlayer() {
    m_stream.stop();
    if (!m_ephemeralPath.isEmpty()) {
        QFile::remove(m_ephemeralPath);
        QFile::remove(m_ephemeralPath + QStringLiteral(".part"));
        QFile::remove(m_ephemeralPath + QStringLiteral(".part.ranges.json"));
    }
}

QVariantList AudioPlayer::bufferedRanges() const {
    if (m_streamingSourceActive) return m_stream.bufferedRanges();
    if (m_bufferProgress >= 1) return {QVariantMap{{QStringLiteral("start"), 0.0},
                                                   {QStringLiteral("end"), 1.0}}};
    return {};
}

void AudioPlayer::setCacheDirectory(const QString &directory) {
    if (directory.trimmed().isEmpty()) return;
    m_cacheDirectory = QDir::cleanPath(QFileInfo(directory).absoluteFilePath());
}

void AudioPlayer::setCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }

void AudioPlayer::setBufferProgress(qreal progress) {
    const qreal clamped = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(m_bufferProgress, clamped)) return;
    m_bufferProgress = clamped;
    emit bufferProgressChanged();
}

void AudioPlayer::setLoading(bool loading) {
    if (m_loading == loading) return;
    m_loading = loading;
    emit loadingChanged();
}

QStringList AudioPlayer::protectedCachePaths() const {
    QStringList paths;
    if (!m_activePlaybackPath.isEmpty()) paths.append(m_activePlaybackPath);
    if (!m_activePartialPath.isEmpty()) paths.append(m_activePartialPath);
    if (!m_stream.rangesPath().isEmpty()) paths.append(m_stream.rangesPath());
    return paths;
}

void AudioPlayer::setVolume(qreal volume) {
    const qreal clamped = qBound(0.0, volume, 1.0);
    if (qFuzzyCompare(static_cast<double>(m_output.volume()), static_cast<double>(clamped))) return;
    m_output.setVolume(clamped);
    emit volumeChanged();
}

void AudioPlayer::restoreSong(const QJsonObject &song, qint64 positionMs) {
    m_failed = false;
    m_advanceWhileLoading = false;
    m_pendingPositionClock.invalidate();
    // Restored selection is paused metadata; opening its source would fetch audio before play.
    m_stream.stop();
    m_streamingSourceActive = false;
    m_song = song;
    m_pendingPosition = qMax<qint64>(0, positionMs);
    m_pendingPlay = false;
    m_playRequested = false;
    m_initialMediaReady = false;
    m_waitingForSeekProgress = false;
    setLoading(false);
    setBufferProgress(0);
    emit songChanged();
    emit durationChanged();
    emit positionChanged();
}

void AudioPlayer::playSong(const QJsonObject &song, qint64 positionMs, bool autoPlay,
                           bool advanceWhileLoading) {
    const QString hash = song.value("hash").toString();
    const QString format = song.value("format").toString();
    if (hash.size() != 64 || !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$")).match(hash).hasMatch() ||
        !QRegularExpression(QStringLiteral("^(mp3|flac|aac|ogg|wav|m4a)$")).match(format).hasMatch()) {
        emit errorOccurred(QStringLiteral("歌曲文件信息无效"));
        return;
    }

    m_stream.stop();
    m_streamingSourceActive = false;
    m_player.stop();
    m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) {
        QFile::remove(m_ephemeralPath);
        QFile::remove(m_ephemeralPath + QStringLiteral(".part"));
        QFile::remove(m_ephemeralPath + QStringLiteral(".part.ranges.json"));
    }
    m_ephemeralPath.clear();
    m_activePartialPath.clear();
    m_activePlaybackPath.clear();
    m_song = song;
    m_failed = false;
    m_advanceWhileLoading = advanceWhileLoading;
    m_pendingPositionClock.invalidate();
    if (autoPlay && advanceWhileLoading) m_pendingPositionClock.start();
    m_pendingPosition = qMax<qint64>(0, positionMs);
    m_pendingPlay = autoPlay;
    m_playRequested = autoPlay;
    m_initialMediaReady = false;
    m_waitingForSeekProgress = false;
    m_lastObservedPosition = m_pendingPosition;
    m_progressClock.restart();
    emit songChanged();
    emit positionChanged();
    setBufferProgress(0);

    const QString persistentDirectory = m_cacheDirectory.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/music_cache"
            : m_cacheDirectory;
    const bool persistCache = m_cacheEnabled;
    const QString directory = persistCache
            ? persistentDirectory
            : QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/FutariMusicPlayback";
    QDir().mkpath(directory);
    const QString persistentPath = persistentDirectory + "/" + hash + "." + format;
    QFile cached(persistentPath);
    if (cached.open(QIODevice::ReadOnly)) {
        const QByteArray digest = hashFile(cached);
        if (digest == hash.toLatin1()) {
            cached.close();
            if (persistCache) pruneCache(persistentDirectory, persistentPath);
            m_activePlaybackPath = persistentPath;
            setLoading(true);
            setBufferProgress(1);
            emit bufferedRangesChanged();
            loadFile(persistentPath, positionMs, autoPlay);
            return;
        }
        cached.close();
        if (persistCache) QFile::remove(persistentPath);
    }

    const qint64 expectedSize = song.value("fileSize").toVariant().toLongLong();
    const QString activeCacheBase = directory + "/" + hash + "." + format;
    if (!persistCache) m_ephemeralPath = activeCacheBase;
    setLoading(true);
    const QUrl source = m_stream.start(m_api->baseUrl(), m_api->token(), hash, format,
                                      expectedSize, directory, persistCache);
    if (!source.isValid() || source.isEmpty()) {
        setLoading(false);
        emit errorOccurred(QStringLiteral("无法启动音乐流，请重试"));
        return;
    }
    m_streamingSourceActive = true;
    m_activePartialPath = m_stream.partialPath();
    m_activePlaybackPath = m_stream.completedPath();
    setBufferProgress(m_stream.bufferProgress());
    emit bufferedRangesChanged();
    m_player.setSource(source);
    // Qt's source loader fetches the first range before the initial seek is applied.
}

void AudioPlayer::loadFile(const QString &path, qint64 positionMs, bool autoPlay) {
    m_initialMediaReady = false;
    m_pendingPosition = qMax<qint64>(0, positionMs);
    m_pendingPlay = autoPlay;
    m_player.setSource(QUrl::fromLocalFile(path));
}

void AudioPlayer::pause() {
    if (!m_initialMediaReady && m_pendingPositionClock.isValid())
        m_pendingPosition += m_pendingPositionClock.elapsed();
    m_pendingPositionClock.invalidate();
    m_pendingPlay = false;
    m_playRequested = false;
    m_waitingForSeekProgress = false;
    setLoading(false);
    m_player.pause();
}

void AudioPlayer::resume() {
    m_playRequested = true;
    if ((m_failed || m_player.source().isEmpty()) && !m_loading && !m_song.isEmpty()) {
        playSong(m_song, m_pendingPosition, true, m_advanceWhileLoading);
    } else if (!m_initialMediaReady) {
        if (!m_pendingPlay && m_advanceWhileLoading) m_pendingPositionClock.start();
        m_pendingPlay = true;
    } else {
        m_player.play();
    }
}

void AudioPlayer::seek(qint64 positionMs) {
    m_pendingPosition = qMax<qint64>(0, positionMs);
    if (!m_initialMediaReady && m_advanceWhileLoading && m_pendingPlay)
        m_pendingPositionClock.start();
    if (m_player.source().isEmpty()) emit positionChanged();
    else if (m_initialMediaReady) {
        if (qAbs(m_player.position() - m_pendingPosition) < kSeekToleranceMs) return;
        m_seekPosition = m_pendingPosition;
        m_lastObservedPosition = m_pendingPosition;
        m_progressClock.restart();
        m_waitingForSeekProgress = m_streamingSourceActive && m_playRequested;
        if (m_waitingForSeekProgress) setLoading(true);
        m_player.setPosition(m_pendingPosition);
    }
}

void AudioPlayer::playLocalFile(const QString &path, const QString &title, const QString &artist) {
    m_failed = false;
    m_advanceWhileLoading = false;
    m_pendingPositionClock.invalidate();
    const QFileInfo file(path);
    if (!file.isFile() || !file.isReadable()) {
        emit errorOccurred(QStringLiteral("本地歌曲文件不存在或无法读取"));
        return;
    }
    m_stream.stop();
    m_streamingSourceActive = false;
    m_player.stop();
    m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) {
        QFile::remove(m_ephemeralPath);
        QFile::remove(m_ephemeralPath + QStringLiteral(".part"));
        QFile::remove(m_ephemeralPath + QStringLiteral(".part.ranges.json"));
    }
    m_ephemeralPath.clear();
    m_activePartialPath.clear();
    m_activePlaybackPath.clear();
    m_song = QJsonObject{{"title", title}, {"artist", artist}, {"localPath", file.absoluteFilePath()}};
    emit songChanged();
    setBufferProgress(1);
    emit bufferedRangesChanged();
    setLoading(true);
    m_initialMediaReady = false;
    m_waitingForSeekProgress = false;
    m_pendingPosition = 0;
    m_pendingPlay = true;
    m_playRequested = true;
    m_player.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
}

void AudioPlayer::stop() {
    m_failed = false;
    m_advanceWhileLoading = false;
    m_pendingPositionClock.invalidate();
    m_stream.stop();
    m_streamingSourceActive = false;
    m_player.stop();
    m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) {
        QFile::remove(m_ephemeralPath);
        QFile::remove(m_ephemeralPath + QStringLiteral(".part"));
        QFile::remove(m_ephemeralPath + QStringLiteral(".part.ranges.json"));
    }
    m_ephemeralPath.clear();
    m_activePlaybackPath.clear();
    m_activePartialPath.clear();
    setBufferProgress(0);
    m_song = {};
    emit songChanged();
    m_pendingPosition = 0;
    m_pendingPlay = false;
    m_playRequested = false;
    m_initialMediaReady = false;
    m_waitingForSeekProgress = false;
    emit positionChanged();
    setLoading(false);
}
