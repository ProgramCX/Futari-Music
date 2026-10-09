#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QRegularExpression>
#include <QStringConverter>
#include <algorithm>

#include "AppController.h"

namespace {
void appendTextPart(QHttpMultiPart* parts, const QString& name, const QString& value) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + name + "\"");
    part.setBody(value.toUtf8());
    parts->append(part);
}
bool appendFilePart(QHttpMultiPart* parts, const QString& name, const QString& url) {
    if (url.isEmpty()) return true;
    const QString path = QUrl(url).toLocalFile();
    auto* file = new QFile(path, parts);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        return false;
    }
    QHttpPart part;
    part.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        "form-data; name=\"" + name + "\"; filename=\"" + QFileInfo(path).fileName() + "\"");
    part.setBodyDevice(file);
    parts->append(part);
    return true;
}
}  // namespace

void AppController::searchSongs(const QString& keyword) {
    resetPage(m_session.songs, keyword.trimmed());
    emit songsChanged();
    loadMoreSongs();
}

void AppController::loadMoreSongs() {
    loadPage(m_session.songs, "api/songs", [this] {
        cacheSongs(m_session.songs.items);
        emit songsChanged();
        fetchCovers();
    });
}

void AppController::refreshAlbums(const QString& keyword) {
    const auto token = m_session.albumsRequests.renew();
    m_api.request("GET",
                  "api/albums?keyword=" + segment(keyword.trimmed()) + "&pageNum=1&pageSize=100",
                  {}, [this, token](const QJsonValue& value) {
                      if (token.expired()) return;
                      m_session.albums = value.toObject().value("list").toArray();
                      emit albumsChanged();
                  });
}

void AppController::refreshUploadedSongs() {
    resetPage(m_session.uploadedSongs, {});
    emit uploadedSongsChanged();
    loadMoreUploadedSongs();
}

void AppController::loadMoreUploadedSongs() {
    if (!authenticated()) return;
    loadPage(m_session.uploadedSongs, "api/songs/mine", [this] {
        cacheSongs(m_session.uploadedSongs.items);
        emit uploadedSongsChanged();
    });
}

void AppController::searchAlbumMatches(const QString& name, const QString& artist) {
    const auto token = m_session.albumSearchRequests.renew();
    const QString cleanedName = name.trimmed();
    if (cleanedName.isEmpty()) {
        m_session.albumMatches = {};
        emit albumMatchesChanged();
        return;
    }
    m_api.request("GET", "api/albums?keyword=" + segment(cleanedName) + "&pageNum=1&pageSize=100",
                  {}, [this, cleanedName, artist, token](const QJsonValue& value) {
                      if (token.expired()) return;
                      selectAlbumMatches(value.toObject().value("list").toArray(), cleanedName,
                                         artist.trimmed());
                  });
}

void AppController::selectAlbumMatches(const QJsonArray& rows, const QString& name,
                                       const QString& artist) {
    QList<QJsonObject> matches;
    for (const auto& row : rows) {
        const QJsonObject album = row.toObject();
        if (QString::compare(album.value("name").toString(), name, Qt::CaseInsensitive) == 0)
            matches.append(album);
    }
    // 同名专辑优先展示同歌手，其他候选保留服务端顺序。
    if (!artist.isEmpty()) {
        std::stable_sort(
            matches.begin(), matches.end(), [&artist](const auto& left, const auto& right) {
                const bool leftMatch = QString::compare(left.value("artist").toString(), artist,
                                                        Qt::CaseInsensitive) == 0;
                const bool rightMatch = QString::compare(right.value("artist").toString(), artist,
                                                         Qt::CaseInsensitive) == 0;
                return leftMatch && !rightMatch;
            });
    }
    m_session.albumMatches = {};
    for (const auto& album : matches) m_session.albumMatches.append(album);
    emit albumMatchesChanged();
}

void AppController::inspectAudio(const QString& audioUrl, const QString& requestId) {
    m_metadataReader.inspect(audioUrl, requestId);
}

QVariantMap AppController::findLyricsForAudio(const QString& audioUrl) const {
    const QString audioPath = QUrl(audioUrl).toLocalFile();
    const QFileInfo audioInfo(audioPath);
    if (!audioInfo.isFile())
        return {{"found", false}, {"message", QStringLiteral("无法读取所选音频目录")}};
    const QString stem = audioInfo.completeBaseName();
    const QFileInfoList files =
        QDir(audioInfo.absolutePath()).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& suffix : {QStringLiteral("lrc"), QStringLiteral("txt")}) {
        for (const QFileInfo& file : files) {
            if (file.suffix().compare(suffix, Qt::CaseInsensitive) == 0 &&
                QString::compare(file.completeBaseName(), stem, Qt::CaseInsensitive) == 0) {
                return {{"found", true},
                        {"url", QUrl::fromLocalFile(file.absoluteFilePath()).toString()},
                        {"name", file.fileName()}};
            }
        }
    }
    return {{"found", false}, {"message", QStringLiteral("未找到同名歌词文件")}};
}

QString AppController::readLyricsFile(const QString& lyricsUrl) const {
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

void AppController::uploadSong(const QString& audioUrl, const QString& title, const QString& artist,
                               qint64 albumId, const QString& newAlbumName, const QString& lyrics,
                               const QString& lyricsUrl, const QString& coverUrl) {
    if (!canUpload()) {
        setError(QStringLiteral("当前账号没有上传权限"));
        return;
    }
    auto* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    appendTextPart(parts, "title", title);
    appendTextPart(parts, "artist", artist);
    if (albumId > 0)
        appendTextPart(parts, "albumId", QString::number(albumId));
    else if (!newAlbumName.trimmed().isEmpty())
        appendTextPart(parts, "newAlbumName", newAlbumName.trimmed());
    if (lyricsUrl.isEmpty()) appendTextPart(parts, "lyricsText", lyrics);
    if (!appendFilePart(parts, "file", audioUrl) ||
        !appendFilePart(parts, "lyricsFile", lyricsUrl) ||
        !appendFilePart(parts, "albumCoverFile", coverUrl)) {
        delete parts;
        setError(QStringLiteral("上传文件无法读取"));
        return;
    }
    m_api.upload("api/songs/upload", parts, [this](const QJsonValue&) {
        emit infoMessage(QStringLiteral("歌曲上传完成"));
        searchSongs({});
        refreshAlbums();
    });
}

void AppController::updateSong(qint64 id, const QString& audioUrl, const QString& title,
                               const QString& artist, qint64 albumId, const QString& albumName,
                               const QString& newAlbumName, const QString& lyrics,
                               const QString& lyricsUrl, const QString& coverUrl) {
    if (!admin()) {
        setError(QStringLiteral("只有管理员可以编辑歌曲"));
        return;
    }
    auto* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    appendTextPart(parts, "title", title);
    appendTextPart(parts, "artist", artist);
    appendTextPart(parts, "albumId", QString::number(albumId));
    if (!newAlbumName.trimmed().isEmpty())
        appendTextPart(parts, "newAlbumName", newAlbumName.trimmed());
    else if (!albumName.isEmpty())
        appendTextPart(parts, "albumName", albumName);
    if (lyricsUrl.isEmpty()) appendTextPart(parts, "lyricsText", lyrics);
    if (!appendFilePart(parts, "file", audioUrl) ||
        !appendFilePart(parts, "lyricsFile", lyricsUrl) ||
        !appendFilePart(parts, "albumCoverFile", coverUrl)) {
        delete parts;
        setError(QStringLiteral("选择的文件无法读取"));
        return;
    }
    const bool albumCoverChanged = !coverUrl.isEmpty();
    m_api.putUpload("api/songs/" + QString::number(id), parts,
                    [this, id, albumCoverChanged](const QJsonValue& value) {
                        invalidateCoverCache(albumCoverChanged ? 0 : id);
                        const QJsonObject updatedSong = value.toObject();
                        if (!updatedSong.isEmpty()) m_session.songCache.insert(id, updatedSong);
                        emit infoMessage(QStringLiteral("歌曲已更新"));
                        searchSongs(m_session.songs.keyword);
                        refreshAlbums();
                        emit songSaved();
                    });
}

void AppController::deleteSong(qint64 id) {
    if (!admin()) {
        setError(QStringLiteral("只有管理员可以删除歌曲"));
        return;
    }
    m_api.request("DELETE", "api/songs/" + QString::number(id), {}, [this, id](const QJsonValue&) {
        invalidateCoverCache(id);
        m_session.songCache.remove(id);
        emit infoMessage(QStringLiteral("歌曲已删除"));
        searchSongs(m_session.songs.keyword);
        emit songDeleted();
    });
}

void AppController::deleteSongs(const QVariantList& songIds) {
    if (!admin()) {
        setError(QStringLiteral("只有管理员可以删除歌曲"));
        return;
    }
    if (m_session.deletingSongs) return;

    QSet<qint64> uniqueIds;
    for (const QVariant& value : songIds) {
        const qint64 id = value.toLongLong();
        if (id > 0) uniqueIds.insert(id);
    }
    for (qint64 id : uniqueIds) m_session.pendingSongDeletions.enqueue(id);
    if (m_session.pendingSongDeletions.isEmpty()) return;

    m_session.deletingSongs = true;
    m_session.songDeletionChangedData = false;
    emit songsChanged();
    deleteNextSong(m_session.songDeletionRequests.renew());
}

void AppController::deleteNextSong(const RequestScope::Token& token) {
    if (token.expired()) return;
    if (m_session.pendingSongDeletions.isEmpty()) {
        const bool deletedAny = m_session.songDeletionChangedData;
        m_session.deletingSongs = false;
        m_session.songDeletionChangedData = false;
        emit songsChanged();
        if (deletedAny) {
            searchSongs(m_session.songs.keyword);
            emit songDeleted();
            emit infoMessage(QStringLiteral("所选歌曲已从曲库删除"));
        }
        return;
    }

    const qint64 id = m_session.pendingSongDeletions.dequeue();
    m_api.request(
        "DELETE", "api/songs/" + QString::number(id), {},
        [this, token, id](const QJsonValue&) {
            if (token.expired()) return;
            invalidateCoverCache(id);
            m_session.songCache.remove(id);
            m_session.songDeletionChangedData = true;
            deleteNextSong(token);
        },
        [this, token] {
            if (token.expired()) return;
            m_session.pendingSongDeletions.clear();
            deleteNextSong(token);
        });
}

QVariantList AppController::guessTrackMetadata(const QString& audioUrl) const {
    QString baseName = QFileInfo(QUrl(audioUrl).toLocalFile()).completeBaseName().trimmed();
    baseName.remove(QRegularExpression(QStringLiteral("^\\s*\\d{1,3}[. _-]+")));
    baseName.replace(QRegularExpression(QStringLiteral("\\s+")), " ");
    if (baseName.isEmpty()) return {};
    QVariantList candidates;
    const QRegularExpression split(QStringLiteral("^(.+?)\\s*(?:[-—–_]|\\bby\\b)\\s*(.+)$"),
                                   QRegularExpression::CaseInsensitiveOption);
    const auto match = split.match(baseName);
    if (match.hasMatch() && !match.captured(1).trimmed().isEmpty() &&
        !match.captured(2).trimmed().isEmpty()) {
        const QString left = match.captured(1).trimmed();
        const QString right = match.captured(2).trimmed();
        candidates.append(
            QVariantMap{{"label", QStringLiteral("歌名在前：%1 · %2").arg(left, right)},
                        {"title", left},
                        {"artist", right}});
        candidates.append(
            QVariantMap{{"label", QStringLiteral("歌手在前：%1 · %2").arg(left, right)},
                        {"title", right},
                        {"artist", left}});
    } else {
        candidates.append(
            QVariantMap{{"label", QStringLiteral("使用文件名作为歌名：%1").arg(baseName)},
                        {"title", baseName},
                        {"artist", QString()}});
    }
    return candidates;
}

void AppController::resetPage(PageState& state, const QString& keyword) {
    state = PageState{};
    state.keyword = keyword;
    state.total = 1;
}

void AppController::loadPage(PageState& state, const QString& path, std::function<void()> publish) {
    if (state.loading || state.items.size() >= state.total) return;
    state.loading = true;
    const auto token = state.requests.token();
    const int page = state.page + 1;
    const ApiClient::PageQuery query{page, 40, state.keyword};
    // state 指向控制器内固定地址的会话成员；先检查 token，再访问被替换的结果集。
    m_api.pagedGet(
        path, query,
        [&state, token, page, publish](const QJsonObject& result) {
            if (token.expired()) return;
            state.total = result.value("total").toInt();
            for (const auto& item : result.value("list").toArray()) state.items.append(item);
            state.page = page;
            state.loading = false;
            publish();
        },
        [&state, token] {
            if (!token.expired()) state.loading = false;
        });
}

void AppController::cacheSongs(const QJsonArray& songs) {
    for (const auto& value : songs) {
        const QJsonObject song = value.toObject();
        const qint64 id = song.value("id").toInteger();
        if (id > 0) m_session.songCache.insert(id, song);
    }
}

void AppController::cachePlaylistSongs(const QJsonArray& playlists) {
    for (const auto& value : playlists) cacheSongs(value.toObject().value("songs").toArray());
}

QJsonObject AppController::findSong(qint64 id) const { return m_session.songCache.value(id); }
