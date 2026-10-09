#include "AudioPlayer.h"
#include "ApiClient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <memory>
#include <algorithm>

namespace {
QByteArray hashFile(QFile &file) {
    QCryptographicHash digest(QCryptographicHash::Sha256);
    while (!file.atEnd()) digest.addData(file.read(64 * 1024));
    return digest.result().toHex();
}
void pruneCache(const QString &directory, const QString &protectedPath) {
    constexpr qint64 maxBytes = 512LL * 1024 * 1024;
    QDir dir(directory);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 total = 0;
    QFileInfoList complete;
    for (const QFileInfo &file : files) {
        if (file.suffix() == "part") {
            if (file.lastModified().daysTo(QDateTime::currentDateTime()) > 7) QFile::remove(file.absoluteFilePath());
            continue;
        }
        total += file.size(); complete.append(file);
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
struct DownloadContext {
    explicit DownloadContext(const QString &path)
        : file(path), digest(QCryptographicHash::Sha256) {}
    QFile file;
    QCryptographicHash digest;
    qint64 offset = 0;
    bool initialized = false;
    bool invalid = false;
};
}

AudioPlayer::AudioPlayer(ApiClient *api, QObject *parent) : QObject(parent), m_api(api) {
    m_cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/music_cache";
    m_player.setAudioOutput(&m_output);
    m_output.setVolume(0.7);
    connect(&m_player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(&m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, [this] { emit playingChanged(); });
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            if (m_loading) { m_loading = false; emit loadingChanged(); }
            m_player.setPosition(m_pendingPosition);
            if (m_pendingPlay) m_player.play();
            m_pendingPlay = false;
        } else if (status == QMediaPlayer::EndOfMedia) emit reachedEnd();
    });
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &description) {
        if (m_loading) { m_loading = false; emit loadingChanged(); }
        m_pendingPlay = false;
        emit errorOccurred(description);
    });
}

void AudioPlayer::setCacheDirectory(const QString &directory) {
    if (directory.trimmed().isEmpty()) return;
    m_cacheDirectory = QDir::cleanPath(QFileInfo(directory).absoluteFilePath());
}

void AudioPlayer::setCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }

QStringList AudioPlayer::protectedCachePaths() const {
    QStringList paths;
    if (!m_activePlaybackPath.isEmpty()) paths.append(m_activePlaybackPath);
    if (!m_activePartialPath.isEmpty()) paths.append(m_activePartialPath);
    return paths;
}

void AudioPlayer::setVolume(qreal volume) {
    const qreal clamped = qBound(0.0, volume, 1.0);
    if (qFuzzyCompare(static_cast<double>(m_output.volume()), static_cast<double>(clamped))) return;
    m_output.setVolume(clamped);
    emit volumeChanged();
}

void AudioPlayer::playSong(const QJsonObject &song, qint64 positionMs, bool autoPlay) {
    const QString hash = song.value("hash").toString();
    const QString format = song.value("format").toString();
    if (hash.size() != 64 || !QRegularExpression(QStringLiteral("^[a-f0-9]{64}$")).match(hash).hasMatch() ||
        !QRegularExpression(QStringLiteral("^(mp3|flac|aac|ogg|wav|m4a)$")).match(format).hasMatch()) {
        emit errorOccurred(QStringLiteral("歌曲文件信息无效"));
        return;
    }
    m_sourceRequests.renew();
    if (m_activeReply) m_activeReply->abort();
    m_player.stop();
    m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) { QFile::remove(m_ephemeralPath); QFile::remove(m_ephemeralPath + ".part"); }
    m_ephemeralPath.clear(); m_activePartialPath.clear(); m_activePlaybackPath.clear();
    m_song = song;
    emit songChanged();
    const QString persistentDirectory = m_cacheDirectory.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/music_cache" : m_cacheDirectory;
    const bool persistCache = m_cacheEnabled;
    QDir().mkpath(persistentDirectory);
    const QString persistentPath = persistentDirectory + "/" + hash + "." + format;
    QFile cached(persistentPath);
    if (cached.open(QIODevice::ReadOnly)) {
        const QByteArray digest = hashFile(cached);
        if (digest == hash.toLatin1()) {
            cached.close();
            if (m_cacheEnabled) pruneCache(persistentDirectory, persistentPath);
            loadFile(persistentPath, positionMs, autoPlay); return;
        }
        cached.close();
        if (m_cacheEnabled) QFile::remove(persistentPath);
    }
    if (m_loading) { m_loading = false; emit loadingChanged(); }
    m_loading = true; emit loadingChanged();
    const auto token = m_sourceRequests.token();
    const QString directory = persistCache ? persistentDirectory
            : QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/FutariMusicPlayback";
    QDir().mkpath(directory);
    const QString path = directory + "/" + hash + "." + format;
    if (!persistCache) m_ephemeralPath = path;
    const QString partialPath = path + ".part";
    m_activePartialPath = partialPath;
    auto context = std::make_shared<DownloadContext>(partialPath);
    if (!context->file.open(QIODevice::ReadWrite)) {
        m_loading = false; emit loadingChanged();
        emit errorOccurred(QStringLiteral("无法写入音乐缓存")); return;
    }
    const qint64 expectedSize = song.value("fileSize").toVariant().toLongLong();
    if (expectedSize > 0 && context->file.size() > expectedSize) context->file.resize(0);
    context->offset = context->file.size();
    while (!context->file.atEnd()) context->digest.addData(context->file.read(64 * 1024));
    context->file.seek(context->offset);
    if (expectedSize > 0 && context->offset == expectedSize && context->digest.result().toHex() == hash.toLatin1()) {
        context->file.close();
        if (QFile::rename(partialPath, path)) {
            m_loading = false; emit loadingChanged();
            if (persistCache) pruneCache(directory, path);
            m_activePartialPath.clear();
            loadFile(path, positionMs, autoPlay); return;
        }
        m_loading = false; emit loadingChanged();
        emit errorOccurred(QStringLiteral("无法完成音乐缓存写入")); return;
    }
    QUrl url = m_api->baseUrl().resolved(QUrl("api/songs/" + hash + "/file"));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_api->token().toUtf8());
    request.setTransferTimeout(120000);
    if (context->offset) request.setRawHeader("Range", "bytes=" + QByteArray::number(context->offset) + "-");
    auto *reply = m_network.get(request);
    m_activeReply = reply;
    const auto consume = [this, reply, context, token] {
        const QByteArray chunk = reply->readAll();
        if (token.expired()) return;
        if (!context->initialized) {
            context->initialized = true;
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (context->offset > 0 && status == 200) {
                context->file.resize(0);
                context->file.seek(0);
                context->digest.reset();
            } else if (context->offset > 0 && status == 206) {
                const QByteArray expected = "bytes " + QByteArray::number(context->offset) + "-";
                if (!reply->rawHeader("Content-Range").startsWith(expected))
                    context->invalid = true;
            } else if (status != 200)
                context->invalid = true;
        }
        if (!context->invalid && !chunk.isEmpty()) {
            context->digest.addData(chunk);
            if (context->file.write(chunk) != chunk.size()) context->invalid = true;
        }
    };
    connect(reply, &QNetworkReply::readyRead, this, consume);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, context, consume, path, partialPath, directory, hash, token, positionMs,
             autoPlay, persistCache] {
                if (m_activeReply == reply) m_activeReply.clear();
                if (token.expired()) {
                    context->file.close();
                    if (!persistCache) QFile::remove(partialPath);
                    reply->deleteLater();
                    return;
                }
                consume();
                context->file.flush();
                context->file.close();
                const bool networkOk =
                    reply->error() == QNetworkReply::NoError &&
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() < 400;
                const bool hashOk = context->digest.result().toHex() == hash.toLatin1();
                bool stored = false;
                if (networkOk && !context->invalid && hashOk) {
                    QFile::remove(path);
                    stored = QFile::rename(partialPath, path);
                }
                if (!stored) {
                    const int status =
                        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    const bool resumable = reply->error() != QNetworkReply::NoError &&
                                           (status == 200 || status == 206) && !context->invalid;
                    if (!resumable) QFile::remove(partialPath);
                    if (!resumable && m_activePartialPath == partialPath)
                        m_activePartialPath.clear();
                    emit errorOccurred(networkOk ? QStringLiteral("下载校验或缓存写入失败")
                                                 : reply->errorString());
                } else {
                    if (persistCache) pruneCache(directory, path);
                    if (m_activePartialPath == partialPath) m_activePartialPath.clear();
                    loadFile(path, positionMs, autoPlay);
                }
                m_loading = false;
                emit loadingChanged();
                reply->deleteLater();
            });
}

void AudioPlayer::loadFile(const QString &path, qint64 positionMs, bool autoPlay) {
    m_activePlaybackPath = path;
    m_pendingPosition = qMax<qint64>(0, positionMs);
    m_pendingPlay = autoPlay;
    m_player.setSource(QUrl::fromLocalFile(path));
}

void AudioPlayer::pause() { m_pendingPlay = false; m_player.pause(); }
void AudioPlayer::resume() { if (!m_loading) m_player.play(); else m_pendingPlay = true; }
void AudioPlayer::seek(qint64 positionMs) {
    m_pendingPosition = qMax<qint64>(0, positionMs);
    if (!m_loading) m_player.setPosition(m_pendingPosition);
}
void AudioPlayer::playLocalFile(const QString &path, const QString &title, const QString &artist) {
    const QFileInfo file(path);
    if (!file.isFile() || !file.isReadable()) {
        emit errorOccurred(QStringLiteral("本地歌曲文件不存在或无法读取"));
        return;
    }
    m_sourceRequests.renew();
    if (m_activeReply) m_activeReply->abort();
    m_player.stop(); m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) { QFile::remove(m_ephemeralPath); QFile::remove(m_ephemeralPath + ".part"); }
    m_ephemeralPath.clear(); m_activePartialPath.clear(); m_activePlaybackPath.clear();
    m_song = QJsonObject{{"title", title}, {"artist", artist}, {"localPath", file.absoluteFilePath()}};
    emit songChanged();
    m_loading = true; emit loadingChanged();
    m_pendingPosition = 0; m_pendingPlay = true;
    m_player.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    m_player.play();
}
void AudioPlayer::stop() {
    m_sourceRequests.renew();
    if (m_activeReply) m_activeReply->abort();
    m_player.stop();
    m_player.setSource({});
    if (!m_ephemeralPath.isEmpty()) {
        QFile::remove(m_ephemeralPath);
        QFile::remove(m_ephemeralPath + ".part");
    }
    m_ephemeralPath.clear(); m_activePlaybackPath.clear(); m_activePartialPath.clear();
    m_song = {}; emit songChanged();
    if (m_loading) { m_loading = false; emit loadingChanged(); }
}
