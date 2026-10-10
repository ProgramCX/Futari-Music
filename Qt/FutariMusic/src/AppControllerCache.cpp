#include "AppController.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>

namespace {
bool pathsOverlap(const QString& left, const QString& right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
    const auto contains = [sensitivity](const QString& parent, const QString& child) {
        return QString::compare(parent, child, sensitivity) == 0 ||
               child.startsWith(parent + QDir::separator(), sensitivity);
    };
    return contains(a, b) || contains(b, a);
}
}  // namespace
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
                               ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                     QStringLiteral("/music_cache")
                               : configured);
}

void AppController::setDownloadDirectory(const QString& directory) {
    const QUrl selected(directory.trimmed());
    const QString trimmed = selected.isLocalFile() ? selected.toLocalFile() : directory.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("请选择下载目录"));
        return;
    }
    const QString path = QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
    if (!QDir().mkpath(path) || !QFileInfo(path).isWritable()) {
        setError(QStringLiteral("下载目录不可写，请选择其他文件夹"));
        return;
    }
    if (pathsOverlap(path, cacheDirectory())) {
        setError(QStringLiteral("下载目录不能与歌曲缓存目录重叠"));
        return;
    }
    m_settings.setValue("downloadDirectory", path);
    emit settingsChanged();
}

void AppController::setCacheDirectory(const QString& directory) {
    const QUrl selected(directory.trimmed());
    const QString trimmed = selected.isLocalFile() ? selected.toLocalFile() : directory.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("请选择缓存目录"));
        return;
    }
    const QString path = QDir::cleanPath(QFileInfo(trimmed).absoluteFilePath());
    if (!QDir().mkpath(path) || !QFileInfo(path).isWritable()) {
        setError(QStringLiteral("缓存目录不可写，请选择其他文件夹"));
        return;
    }
    if (pathsOverlap(path, downloadDirectory())) {
        setError(QStringLiteral("缓存目录不能与下载目录重叠，以免清理缓存时误删下载歌曲"));
        return;
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
    QDirIterator iterator(cacheDirectory(), QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isSymLink()) total += iterator.fileInfo().size();
    }
    return total;
}

QString AppController::enqueueUpload(const QVariantMap& metadata) {
    if (!canUpload()) {
        setError(QStringLiteral("当前账号没有歌曲上传权限"));
        return {};
    }
    const QString id = m_transfers.enqueueUpload(metadata);
    if (id.isEmpty()) setError(QStringLiteral("无法创建上传任务，请检查音频文件"));
    return id;
}

void AppController::enqueueDownload(const QVariantMap& song) {
    if (!authenticated()) {
        setError(QStringLiteral("请先登录后下载歌曲"));
        return;
    }
    if (m_transfers.enqueueDownload(song, downloadDirectory()).isEmpty())
        setError(QStringLiteral("无法创建下载任务，请检查歌曲文件或下载目录"));
}

void AppController::openLocalFolder(const QString& path) {
    const QFileInfo info(path);
    const QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    if (!QFileInfo::exists(folder) || !QDesktopServices::openUrl(QUrl::fromLocalFile(folder)))
        setError(QStringLiteral("无法打开此文件夹"));
}

void AppController::clearSongCache() {
    if (pathsOverlap(cacheDirectory(), downloadDirectory())) {
        setError(QStringLiteral("缓存目录与下载目录重叠，已停止清理"));
        return;
    }
    const QString directoryPath = cacheDirectory();
    QDir directory(directoryPath);
    const QStringList protectedPaths = m_player.protectedCachePaths();
    static const QRegularExpression cacheFilePattern(
        QStringLiteral("^[a-f0-9]{64}\\.(mp3|flac|aac|ogg|wav|m4a)(\\.part)?$"));
    for (const QFileInfo& file : directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (file.isSymLink() || !cacheFilePattern.match(file.fileName()).hasMatch()) continue;
        bool protectedFile = false;
        for (const QString& path : protectedPaths)
            if (QString::compare(QDir::cleanPath(file.absoluteFilePath()), QDir::cleanPath(path),
                                 Qt::CaseInsensitive) == 0)
                protectedFile = true;
        if (!protectedFile) QFile::remove(file.absoluteFilePath());
    }
    emit settingsChanged();
    emit infoMessage(QStringLiteral("已清理未使用的歌曲缓存"));
}

void AppController::fetchCovers() {
    for (const QJsonValue& value : m_session.songs.items) fetchCover(value.toObject());
}

void AppController::onCoversCompleted() {
    invalidateCoverCache();
    searchSongs(m_session.songs.keyword);
    refreshAlbums();
    refreshUploadedSongs();
    refreshPlaylists();
    refreshServerPlaylists();
    const qint64 songId = m_player.song().value("id").toLongLong();
    if (songId <= 0) return;
    const auto token = m_session.coverRequests.token();
    // 正在播放的歌曲可能不在当前分页内，单独刷新封面，不重启音频。
    m_api.request("GET", "api/songs/" + QString::number(songId), {},
                  [this, token](const QJsonValue& value) {
                      if (token.expired()) return;
                      const auto song = value.toObject();
                      m_session.songCache.insert(song.value("id").toInteger(), song);
                      fetchCover(song);
                      emit songsChanged();
                  });
}

void AppController::fetchCover(const QJsonObject& song) {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
    QDir().mkpath(directory);
    const QString id = QString::number(song.value("id").toInteger());
    if (song.value("coverUrl").isNull() || m_session.covers.contains(id) ||
        m_session.coverLoading.contains(id))
        return;
    const QString path = directory + "/" + id + ".image";
    if (QFile::exists(path)) {
        m_session.covers.insert(id, QUrl::fromLocalFile(path).toString());
        emit coversChanged();
        pruneCoverCache(path);
        return;
    }
    m_session.coverLoading.insert(id);
    const auto token = m_session.coverRequests.token();
    m_api.download(
        "api/songs/" + id + "/cover", [this, path, id, token](const QJsonValue& encoded) {
            if (token.expired()) return;
            m_session.coverLoading.remove(id);
            const QByteArray bytes = QByteArray::fromBase64(encoded.toString().toLatin1());
            QSaveFile file(path);
            if (!bytes.isEmpty() && file.open(QIODevice::WriteOnly) &&
                file.write(bytes) == bytes.size() && file.commit()) {
                m_session.covers.insert(id, QUrl::fromLocalFile(path).toString());
                emit coversChanged();
                pruneCoverCache(path);
            }
        });
}

void AppController::invalidateCoverCache(qint64 songId) {
    m_session.coverRequests.renew();
    m_session.coverLoading.clear();
    const QString directoryPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
    if (songId > 0) {
        const QString id = QString::number(songId);
        m_session.covers.remove(id);
        QFile::remove(directoryPath + "/" + id + ".image");
    } else {
        m_session.covers.clear();
        QDir directory(directoryPath);
        for (const QString& fileName : directory.entryList({"*.image"}, QDir::Files))
            directory.remove(fileName);
    }
    emit coversChanged();
}

void AppController::pruneCoverCache(const QString& protectedPath) {
    constexpr qint64 kMaxCoverCacheBytes = 128LL * 1024 * 1024;
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers");
    QFileInfoList files = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 total = 0;
    for (const QFileInfo& file : files) total += file.size();
    std::sort(files.begin(), files.end(), [](const QFileInfo& left, const QFileInfo& right) {
        return left.lastModified() < right.lastModified();
    });
    for (const QFileInfo& file : files) {
        if (total <= kMaxCoverCacheBytes) break;
        if (file.absoluteFilePath() == protectedPath) continue;
        if (QFile::remove(file.absoluteFilePath())) {
            total -= file.size();
            m_session.covers.remove(file.baseName());
            emit coversChanged();
        }
    }
}
