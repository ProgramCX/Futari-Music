#include "TransferManager.h"
#include "MultipartRequest.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrlQuery>
#include <QUuid>
#include <algorithm>
#include <utility>

struct DownloadState {
    explicit DownloadState(QString partialPath) : file(std::move(partialPath)), digest(QCryptographicHash::Sha256) {}
    QFile file;
    QCryptographicHash digest;
    qint64 offset = 0;
    qint64 totalBytes = 0;
    bool initialized = false;
    bool invalid = false;
};

namespace {
QString localPath(const QString &url) {
    const QUrl parsed(url);
    return parsed.isLocalFile() ? parsed.toLocalFile() : url;
}

void appendText(QHttpMultiPart *parts, const QString &name, const QString &value) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + name.toUtf8() + "\"");
    part.setBody(value.toUtf8());
    parts->append(part);
}

bool appendFile(QHttpMultiPart *parts, const QString &name, const QString &path) {
    if (path.isEmpty()) return true;
    const QString local = localPath(path);
    auto *file = new QFile(local, parts);
    if (!file->open(QIODevice::ReadOnly)) { delete file; return false; }
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   "form-data; name=\"" + name.toUtf8() + "\"; filename=\"" + QFileInfo(local).fileName().toUtf8() + "\"");
    part.setBodyDevice(file);
    parts->append(part);
    return true;
}

QByteArray hashFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) hash.addData(file.read(128 * 1024));
    return hash.result().toHex();
}

QString safeName(QString value) {
    value.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1F]")), QStringLiteral("_"));
    value = value.trimmed();
    while (value.endsWith(QLatin1Char('.')) || value.endsWith(QLatin1Char(' '))) value.chop(1);
    if (value.isEmpty()) value = QStringLiteral("未命名歌曲");
    const QString stem = value.section(QLatin1Char('.'), 0, 0).toUpper();
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
        QRegularExpression(QStringLiteral("^(COM|LPT)[1-9]$")).match(stem).hasMatch()) value.prepend(QLatin1Char('_'));
    if (value.size() > 160) value = value.left(160).trimmed();
    return value;
}

QString uniqueTargetPath(const QString &directory, const QString &baseName, const QString &extension) {
    QString candidate = QDir(directory).filePath(baseName + QLatin1Char('.') + extension);
    int suffix = 2;
    while (QFileInfo::exists(candidate) || QFileInfo::exists(candidate + QStringLiteral(".part")))
        candidate = QDir(directory).filePath(baseName + QStringLiteral(" (%1).%2").arg(suffix++).arg(extension));
    return candidate;
}

bool terminalStatus(const QString &status) {
    return status == "success" || status == "cancelled";
}
}

TransferManager::TransferManager(QObject *parent)
    : QObject(parent), m_settings(QSettings::defaultFormat(), QSettings::UserScope, "Futari", "FutariMusic") {}

QVariantList TransferManager::tasks() const {
    QVariantList result;
    result.reserve(m_tasks.size());
    for (QVariantMap task : m_tasks) {
        if (task.value("kind").toString() == "download" && task.value("status").toString() == "success")
            task.insert("fileExists", QFileInfo::exists(task.value("targetPath").toString()));
        result.append(task);
    }
    return result;
}

void TransferManager::setContext(const QUrl &baseUrl, const QString &token, qint64 userId) {
    const bool userChanged = userId != m_userId;
    if (userChanged && m_userId > 0) persist();
    if (userChanged || token.isEmpty() || (!m_token.isEmpty() && token != m_token)) {
        for (auto it = m_replies.cbegin(); it != m_replies.cend(); ++it) {
            const int index = indexOf(it.key());
            if (index >= 0 && (m_tasks[index].value("status").toString() == "uploading" || m_tasks[index].value("status").toString() == "downloading"))
                markInterrupted(it.key(), QStringLiteral("登录状态已结束；任务可在重新登录后继续"));
            if (it.value()) it.value()->abort();
        }
        m_replies.clear();
        m_uploadActiveId.clear();
    }
    m_baseUrl = baseUrl;
    m_token = token;
    if (userChanged) {
        m_userId = userId;
        m_tasks.clear();
        loadForCurrentUser();
    } else m_userId = userId;
    changed();
    processQueues();
}

QString TransferManager::enqueueUpload(const QVariantMap &metadata) {
    const QString audioPath = localPath(metadata.value("audioUrl").toString());
    const QFileInfo audio(audioPath);
    if (m_userId <= 0 || !audio.isFile() || audio.size() <= 0) return {};
    for (const QVariantMap &task : m_tasks) {
        if (task.value("kind").toString() == "upload" && task.value("audioPath").toString() == audio.absoluteFilePath() &&
            !terminalStatus(task.value("status").toString()) && task.value("status").toString() != "failed")
            return task.value("id").toString();
    }
    QVariantMap task = metadata;
    task.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    task.insert("kind", QStringLiteral("upload"));
    task.insert("status", QStringLiteral("waiting"));
    task.insert("title", metadata.value("title").toString().trimmed());
    task.insert("artist", metadata.value("artist").toString().trimmed());
    task.insert("album", metadata.value("album").toString().trimmed());
    task.insert("audioPath", audio.absoluteFilePath());
    task.insert("fileSize", audio.size());
    task.insert("transferred", 0);
    task.insert("totalBytes", audio.size());
    task.insert("progress", 0);
    task.insert("speedBytes", 0);
    task.insert("createdAt", QDateTime::currentMSecsSinceEpoch());
    task.insert("message", QString());
    const QString id = task.value("id").toString();
    m_tasks.append(task);
    changed();
    processQueues();
    return id;
}

QString TransferManager::enqueueDownload(const QVariantMap &song, const QString &downloadDirectory) {
    if (m_userId <= 0) return {};
    const QString hash = song.value("hash").toString().toLower();
    const QString format = song.value("format").toString().toLower();
    if (!QRegularExpression(QStringLiteral("^[a-f0-9]{64}$")).match(hash).hasMatch() ||
        !QRegularExpression(QStringLiteral("^(mp3|flac|aac|ogg|wav|m4a)$")).match(format).hasMatch()) return {};
    const QString directory = QDir::cleanPath(QFileInfo(downloadDirectory).absoluteFilePath());
    if (!QDir().mkpath(directory)) return {};
    const QString display = safeName((song.value("artist").toString().isEmpty() ? QString() : song.value("artist").toString() + QStringLiteral(" - ")) + song.value("title").toString());
    QString target = uniqueTargetPath(directory, display, format);
    const QString expectedFinal = QDir(directory).filePath(display + QLatin1Char('.') + format);
    if (QFileInfo::exists(expectedFinal) && hashFile(expectedFinal) == hash.toLatin1()) target = expectedFinal;

    for (const QVariantMap &existing : m_tasks) {
        if (existing.value("kind").toString() == "download" && existing.value("hash").toString() == hash &&
            existing.value("status").toString() != "cancelled") {
            if (existing.value("status").toString() == "success" &&
                !QFileInfo::exists(existing.value("targetPath").toString())) {
                const int existingIndex = indexOf(existing.value("id").toString());
                if (existingIndex >= 0) {
                    m_tasks[existingIndex].insert("status", QStringLiteral("waiting"));
                    m_tasks[existingIndex].insert("message", QStringLiteral("本地文件已移除，重新下载中"));
                    m_tasks[existingIndex].insert("directory", directory);
                    m_tasks[existingIndex].insert("targetPath", uniqueTargetPath(directory, display, format));
                    m_tasks[existingIndex].insert("transferred", 0);
                    m_tasks[existingIndex].insert("progress", 0);
                    m_tasks[existingIndex].insert("fileExists", false);
                    changed(); processQueues();
                }
            }
            return existing.value("id").toString();
        }
    }

    QVariantMap task = song;
    task.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    task.insert("kind", QStringLiteral("download"));
    task.insert("status", QStringLiteral("waiting"));
    task.insert("directory", directory);
    task.insert("targetPath", target);
    task.insert("fileSize", song.value("fileSize").toLongLong());
    task.insert("transferred", 0);
    task.insert("totalBytes", song.value("fileSize").toLongLong());
    task.insert("progress", 0);
    task.insert("speedBytes", 0);
    task.insert("createdAt", QDateTime::currentMSecsSinceEpoch());
    task.insert("message", QString());
    const QString id = task.value("id").toString();
    if (target == expectedFinal && QFileInfo::exists(expectedFinal) && hashFile(expectedFinal) == hash.toLatin1()) {
        task.insert("status", QStringLiteral("success"));
        task.insert("progress", 100);
        task.insert("transferred", QFileInfo(expectedFinal).size());
        task.insert("totalBytes", QFileInfo(expectedFinal).size());
        task.insert("message", QStringLiteral("文件已存在且内容一致，已复用本地文件"));
        task.insert("completedAt", QDateTime::currentMSecsSinceEpoch());
    }
    m_tasks.append(task);
    changed();
    if (task.value("status").toString() == "success") emit taskSucceeded(id, song, QStringLiteral("download"));
    else processQueues();
    return id;
}

int TransferManager::indexOf(const QString &taskId) const {
    for (int i = 0; i < m_tasks.size(); ++i)
        if (m_tasks[i].value("id").toString() == taskId) return i;
    return -1;
}

void TransferManager::persist() {
    if (m_userId <= 0) return;
    QJsonArray rows;
    for (const QVariantMap &task : m_tasks) rows.append(QJsonObject::fromVariantMap(task));
    m_settings.setValue(QStringLiteral("transferTasks/%1").arg(m_userId), QJsonDocument(rows).toJson(QJsonDocument::Compact));
    m_settings.sync();
}

void TransferManager::loadForCurrentUser() {
    if (m_userId <= 0) return;
    const QByteArray saved = m_settings.value(QStringLiteral("transferTasks/%1").arg(m_userId)).toByteArray();
    const QJsonDocument document = QJsonDocument::fromJson(saved);
    if (!document.isArray()) return;
    for (const QJsonValue &value : document.array()) {
        QVariantMap task = value.toObject().toVariantMap();
        const QString status = task.value("status").toString();
        if (status == "cancelled") {
            if (task.value("kind").toString() == "download")
                QFile::remove(task.value("targetPath").toString() + QStringLiteral(".part"));
            continue;
        }
        if (status == "uploading" || status == "downloading") {
            task.insert("status", QStringLiteral("interrupted"));
            task.insert("message", QStringLiteral("应用退出时传输未完成，可继续或重试"));
        }
        if (task.value("kind").toString() == "download" && task.value("status").toString() == "success") {
            const QString path = task.value("targetPath").toString();
            task.insert("fileExists", QFileInfo::exists(path));
        }
        m_tasks.append(task);
    }
}

void TransferManager::changed(bool save) {
    if (save) persist();
    emit tasksChanged();
}

QNetworkRequest TransferManager::makeRequest(const QString &path) const {
    const QUrl url = m_baseUrl.resolved(QUrl(path.startsWith('/') ? path.mid(1) : path));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setTransferTimeout(120000);
    return request;
}

void TransferManager::processQueues() {
    if (m_suppressQueueProcessing || m_userId <= 0 || m_token.isEmpty() || m_baseUrl.isEmpty()) return;
    if (m_uploadActiveId.isEmpty()) {
        for (const QVariantMap &task : std::as_const(m_tasks)) {
            if (task.value("kind").toString() == "upload" && task.value("status").toString() == "waiting") {
                startUpload(task.value("id").toString());
                break;
            }
        }
    }
    int activeDownloads = 0;
    for (const QVariantMap &task : std::as_const(m_tasks))
        if (task.value("kind").toString() == "download" && task.value("status").toString() == "downloading") ++activeDownloads;
    activeDownloads += m_cancelledDownloadPaths.size();
    for (const QVariantMap &task : std::as_const(m_tasks)) {
        if (activeDownloads >= 2) break;
        if (task.value("kind").toString() == "download" && task.value("status").toString() == "waiting") {
            startDownload(task.value("id").toString());
            ++activeDownloads;
        }
    }
}

void TransferManager::startUpload(const QString &taskId) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    QVariantMap &task = m_tasks[index];
    const QString audioPath = task.value("audioPath").toString();
    if (!QFileInfo(audioPath).isFile()) {
        task.insert("status", QStringLiteral("failed"));
        task.insert("message", QStringLiteral("本地音频文件不存在，请重新选择文件后添加上传任务"));
        changed(); processQueues(); return;
    }
    auto *parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    bool valid = true;
    appendText(parts, QStringLiteral("title"), task.value("title").toString());
    appendText(parts, QStringLiteral("artist"), task.value("artist").toString());
    const qint64 durationMs = task.value("durationMs").toLongLong();
    if (durationMs > 0) appendText(parts, QStringLiteral("durationMs"), QString::number(durationMs));
    const qint64 albumId = task.value("albumId").toLongLong();
    const QString newAlbumName = task.value("newAlbumName").toString().trimmed();
    if (albumId > 0) appendText(parts, QStringLiteral("albumId"), QString::number(albumId));
    else if (!newAlbumName.isEmpty()) {
        appendText(parts, QStringLiteral("newAlbumName"), newAlbumName);
        appendText(parts, QStringLiteral("newAlbumArtist"), task.value("newAlbumArtist").toString());
    }
    if (task.contains("lyricsText")) appendText(parts, QStringLiteral("lyricsText"), task.value("lyricsText").toString());
    const QString albumCoverPath = task.value("albumCoverPath").toString();
    const QString songCoverPath = task.value("songCoverPath").toString();
    valid = appendFile(parts, QStringLiteral("albumCoverFile"), albumCoverPath) && valid;
    valid = appendFile(parts, QStringLiteral("songCoverFile"), songCoverPath) && valid;
    valid = appendFile(parts, QStringLiteral("file"), audioPath) && valid;
    if (!valid) {
        delete parts;
        task.insert("status", QStringLiteral("failed"));
        task.insert("message", QStringLiteral("无法打开上传文件，请检查文件是否被移动或占用"));
        changed(); processQueues(); return;
    }
    task.insert("status", QStringLiteral("uploading"));
    task.insert("message", QString());
    task.insert("transferred", 0);
    task.insert("progress", 0);
    task.insert("speedBytes", 0);
    m_uploadActiveId = taskId;
    changed();
    QNetworkRequest request = makeRequest(QStringLiteral("api/songs/upload"));
    setMultipartContentType(request, *parts);
    QNetworkReply *reply = m_network.post(request, parts);
    parts->setParent(reply);
    m_replies.insert(taskId, reply);
    m_lastProgressTime.insert(taskId, QDateTime::currentMSecsSinceEpoch());
    m_lastProgressBytes.insert(taskId, 0);
    connect(reply, &QNetworkReply::uploadProgress, this, [this, taskId](qint64 sent, qint64 total) {
        updateProgress(taskId, sent, total);
    });
    connect(reply, &QNetworkReply::finished, this, [this, taskId, reply] { finishUpload(taskId, reply); });
}

void TransferManager::finishUpload(const QString &taskId, QNetworkReply *reply) {
    const QByteArray body = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    const QJsonObject envelope = document.object();
    const int index = indexOf(taskId);
    QVariantMap uploadedSong;
    if (index >= 0) {
        QVariantMap &task = m_tasks[index];
        const QString taskStatus = task.value("status").toString();
        if (taskStatus != "paused" && taskStatus != "cancelled" && taskStatus != "interrupted") {
            if (statusCode == 401) emit unauthorized();
            if (reply->error() == QNetworkReply::NoError && statusCode < 400 && parseError.error == QJsonParseError::NoError && envelope.value("code").toInt(-1) == 0) {
                uploadedSong = envelope.value("data").toObject().toVariantMap();
                task.insert("song", uploadedSong);
                task.insert("status", QStringLiteral("success"));
                task.insert("progress", 100);
                task.insert("transferred", task.value("fileSize"));
                task.insert("totalBytes", task.value("fileSize"));
                task.insert("completedAt", QDateTime::currentMSecsSinceEpoch());
                task.insert("message", QString());
            } else {
                task.insert("status", QStringLiteral("failed"));
                const QString serverMessage = envelope.value("message").toString();
                QString failureMessage = serverMessage;
                if (failureMessage.isEmpty() && statusCode == 413)
                    failureMessage = QStringLiteral("上传内容超过服务器配置的大小限制，请调整文件大小或联系管理员");
                if (failureMessage.isEmpty() && statusCode > 0)
                    failureMessage = QStringLiteral("服务端返回 HTTP %1：%2").arg(statusCode).arg(reply->errorString());
                if (failureMessage.isEmpty()) failureMessage = reply->errorString();
                task.insert("message", failureMessage);
            }
        }
    }
    m_replies.remove(taskId);
    m_lastProgressTime.remove(taskId); m_lastProgressBytes.remove(taskId); m_lastPersistTime.remove(taskId);
    if (m_uploadActiveId == taskId) m_uploadActiveId.clear();
    reply->deleteLater();
    changed();
    if (!uploadedSong.isEmpty()) emit taskSucceeded(taskId, uploadedSong, QStringLiteral("upload"));
    processQueues();
}

void TransferManager::startDownload(const QString &taskId) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    QVariantMap &task = m_tasks[index];
    const QString directory = QDir::cleanPath(task.value("directory").toString());
    QString target = QDir::cleanPath(task.value("targetPath").toString());
    if (QDir::cleanPath(QFileInfo(target).absolutePath()) != QDir::cleanPath(QFileInfo(directory).absoluteFilePath())) {
        task.insert("status", QStringLiteral("failed")); task.insert("message", QStringLiteral("下载目标路径无效")); changed(); return;
    }
    const QString hash = task.value("hash").toString().toLower();
    const QString format = task.value("format").toString().toLower();
    const QString partialPath = target + QStringLiteral(".part");
    if (QFileInfo::exists(target)) {
        if (hashFile(target) == hash.toLatin1()) {
            task.insert("status", QStringLiteral("success")); task.insert("progress", 100);
            task.insert("transferred", QFileInfo(target).size()); task.insert("totalBytes", QFileInfo(target).size());
            task.insert("fileExists", true); task.insert("completedAt", QDateTime::currentMSecsSinceEpoch());
            changed(); emit taskSucceeded(taskId, task, QStringLiteral("download")); return;
        }
        const QString safeBase = QFileInfo(target).completeBaseName();
        target = uniqueTargetPath(directory, safeBase, format);
        task.insert("targetPath", target);
    }
    auto state = std::make_shared<DownloadState>(target + QStringLiteral(".part"));
    if (!state->file.open(QIODevice::ReadWrite)) {
        task.insert("status", QStringLiteral("failed")); task.insert("message", QStringLiteral("下载目录不可写")); changed(); return;
    }
    state->offset = state->file.size();
    while (!state->file.atEnd()) state->digest.addData(state->file.read(128 * 1024));
    const qint64 expectedSize = task.value("fileSize").toLongLong();
    if (expectedSize > 0 && state->offset > expectedSize) { state->file.resize(0); state->digest.reset(); state->offset = 0; }
    if (expectedSize > 0 && state->offset == expectedSize && state->digest.result().toHex() == hash.toLatin1()) {
        state->file.close();
        if (QFile::rename(partialPath, target)) {
            task.insert("status", QStringLiteral("success")); task.insert("progress", 100);
            task.insert("transferred", expectedSize); task.insert("totalBytes", expectedSize);
            task.insert("fileExists", true); task.insert("completedAt", QDateTime::currentMSecsSinceEpoch());
            changed(); emit taskSucceeded(taskId, task, QStringLiteral("download")); return;
        }
    } else if (expectedSize > 0 && state->offset == expectedSize) {
        state->file.resize(0); state->digest.reset(); state->offset = 0;
    }
    state->file.seek(state->offset);
    task.insert("status", QStringLiteral("downloading"));
    task.insert("transferred", state->offset);
    task.insert("totalBytes", expectedSize);
    task.insert("progress", expectedSize > 0 ? qBound(0, static_cast<int>(state->offset * 100 / expectedSize), 99) : 0);
    task.insert("speedBytes", 0); task.insert("message", QString());
    changed();
    const QString songHash = task.value("hash").toString();
    QNetworkRequest request = makeRequest(QStringLiteral("api/songs/%1/file").arg(songHash));
    if (state->offset > 0) request.setRawHeader("Range", "bytes=" + QByteArray::number(state->offset) + "-");
    QNetworkReply *reply = m_network.get(request);
    m_replies.insert(taskId, reply);
    m_lastProgressTime.insert(taskId, QDateTime::currentMSecsSinceEpoch());
    m_lastProgressBytes.insert(taskId, state->offset);
    const auto prepare = [reply, state] {
        if (state->initialized) return;
        state->initialized = true;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (state->offset > 0 && status == 200) {
            state->file.resize(0); state->file.seek(0); state->digest.reset(); state->offset = 0;
        } else if (state->offset > 0 && status == 206) {
            static const QRegularExpression rangePattern(QStringLiteral("^bytes (\\d+)-(\\d+)/(\\d+)$"));
            const auto match = rangePattern.match(QString::fromLatin1(reply->rawHeader("Content-Range")));
            if (!match.hasMatch() || match.captured(1).toLongLong() != state->offset) state->invalid = true;
            else state->totalBytes = match.captured(3).toLongLong();
        } else if (status != 200) state->invalid = true;
        if (state->totalBytes <= 0) {
            const qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
            if (contentLength > 0) state->totalBytes = state->offset + contentLength;
        }
    };
    const auto consume = [this, taskId, reply, state, prepare] {
        prepare();
        const QByteArray bytes = reply->readAll();
        if (!state->invalid && !bytes.isEmpty()) {
            if (state->file.write(bytes) != bytes.size()) state->invalid = true;
            else state->digest.addData(bytes);
        }
        if (!state->invalid) updateProgress(taskId, state->offset + state->file.size() - state->offset,
                                            state->totalBytes > 0 ? state->totalBytes : state->file.size());
    };
    connect(reply, &QNetworkReply::metaDataChanged, this, prepare);
    connect(reply, &QNetworkReply::readyRead, this, consume);
    connect(reply, &QNetworkReply::downloadProgress, this, [this, taskId, state](qint64 received, qint64 total) {
        updateProgress(taskId, state->offset + received, state->totalBytes > 0 ? state->totalBytes : state->offset + total);
    });
    connect(reply, &QNetworkReply::finished, this, [this, taskId, reply, state, consume, prepare] {
        prepare(); consume(); finishDownload(taskId, reply, state);
    });
}

void TransferManager::finishDownload(const QString &taskId, QNetworkReply *reply,
                                     const std::shared_ptr<DownloadState> &state) {
    const int index = indexOf(taskId);
    state->file.flush();
    state->file.close();
    const QString cancelledPartialPath = m_cancelledDownloadPaths.take(taskId);
    if (!cancelledPartialPath.isEmpty()) {
        QFile::remove(cancelledPartialPath);
    } else if (index >= 0) {
        QVariantMap &task = m_tasks[index];
        const QString status = task.value("status").toString();
        const QString target = task.value("targetPath").toString();
        const QString partialPath = target + QStringLiteral(".part");
        const qint64 downloadedSize = QFileInfo(partialPath).size();
        if (status == "cancelled") QFile::remove(partialPath);
        else if (status != "paused" && status != "interrupted") {
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatus == 401) emit unauthorized();
            const bool httpOk = reply->error() == QNetworkReply::NoError && httpStatus < 400;
            const QString expectedHash = task.value("hash").toString().toLower();
            const bool hashOk = state->digest.result().toHex() == expectedHash.toLatin1();
            const qint64 expectedSize = task.value("fileSize").toLongLong();
            const bool sizeOk = expectedSize <= 0 || downloadedSize == expectedSize;
            if (!httpOk) {
                task.insert("status", QStringLiteral("failed")); task.insert("message", reply->errorString());
            } else if (state->invalid || !hashOk || !sizeOk) {
                task.insert("status", QStringLiteral("failed"));
                task.insert("message", QStringLiteral("下载内容校验失败，可重试"));
                if (!sizeOk && downloadedSize < expectedSize) {
                    task.insert("transferred", downloadedSize);
                } else QFile::remove(partialPath);
            } else if (!QFile::rename(partialPath, target)) {
                task.insert("status", QStringLiteral("failed")); task.insert("message", QStringLiteral("无法保存下载文件，请检查目录权限"));
            } else {
                task.insert("status", QStringLiteral("success")); task.insert("progress", 100);
                task.insert("transferred", QFileInfo(target).size()); task.insert("totalBytes", QFileInfo(target).size());
                task.insert("fileExists", true); task.insert("completedAt", QDateTime::currentMSecsSinceEpoch());
                task.insert("message", QString());
                emit taskSucceeded(taskId, task, QStringLiteral("download"));
            }
        }
    }
    m_replies.remove(taskId);
    m_lastProgressTime.remove(taskId); m_lastProgressBytes.remove(taskId); m_lastPersistTime.remove(taskId);
    reply->deleteLater();
    changed();
    processQueues();
}

void TransferManager::updateProgress(const QString &taskId, qint64 transferred, qint64 total) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    QVariantMap &task = m_tasks[index];
    task.insert("transferred", transferred);
    if (total > 0) task.insert("totalBytes", total);
    const qint64 safeTotal = task.value("totalBytes").toLongLong();
    if (safeTotal > 0) task.insert("progress", qBound(0, static_cast<int>(transferred * 100 / safeTotal), 99));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 previousTime = m_lastProgressTime.value(taskId, now);
    const qint64 elapsed = now - previousTime;
    if (elapsed >= 300) {
        const qint64 previousBytes = m_lastProgressBytes.value(taskId, transferred);
        task.insert("speedBytes", qMax<qint64>(0, (transferred - previousBytes) * 1000 / elapsed));
        m_lastProgressTime.insert(taskId, now); m_lastProgressBytes.insert(taskId, transferred);
    }
    emit tasksChanged();
    if (now - m_lastPersistTime.value(taskId, 0) >= 1000) {
        m_lastPersistTime.insert(taskId, now); persist();
    }
}

void TransferManager::markInterrupted(const QString &taskId, const QString &message) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    m_tasks[index].insert("status", QStringLiteral("interrupted"));
    m_tasks[index].insert("message", message);
}

void TransferManager::pauseTask(const QString &taskId) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    QVariantMap &task = m_tasks[index];
    const QString kind = task.value("kind").toString();
    const QString status = task.value("status").toString();
    if (status != "waiting" && status != "uploading" && status != "downloading") return;
    task.insert("status", QStringLiteral("paused"));
    task.insert("speedBytes", 0);
    if (kind == "upload") {
        task.insert("transferred", 0); task.insert("progress", 0);
        task.insert("message", QStringLiteral("上传请求已中止；继续时将从头开始上传"));
    } else task.insert("message", QStringLiteral("网络请求已中止，已写入部分文件会在继续时校验并续传"));
    changed();
    if (m_replies.value(taskId)) m_replies.value(taskId)->abort();
    processQueues();
}

void TransferManager::resumeTask(const QString &taskId) {
    const int index = indexOf(taskId);
    if (index < 0) return;
    QVariantMap &task = m_tasks[index];
    const QString status = task.value("status").toString();
    if (status != "paused" && status != "interrupted" && status != "failed") return;
    if (task.value("kind").toString() == "upload") { task.insert("transferred", 0); task.insert("progress", 0); }
    task.insert("status", QStringLiteral("waiting")); task.insert("message", QString());
    changed(); processQueues();
}

void TransferManager::retryTask(const QString &taskId) { resumeTask(taskId); }

void TransferManager::cancelTask(const QString &taskId) {
    const int index = indexOf(taskId);
    if (index < 0 || terminalStatus(m_tasks[index].value("status").toString())) return;
    const QVariantMap task = m_tasks.at(index);
    const QPointer<QNetworkReply> reply = m_replies.value(taskId);
    m_tasks.removeAt(index);
    if (task.value("kind").toString() == "download") {
        const QString partialPath = task.value("targetPath").toString() + QStringLiteral(".part");
        if (reply) m_cancelledDownloadPaths.insert(taskId, partialPath);
        else QFile::remove(partialPath);
    }
    changed();
    if (reply) reply->abort();
    else {
        if (m_uploadActiveId == taskId) m_uploadActiveId.clear();
        processQueues();
    }
}

void TransferManager::pauseAll(const QString &kind) {
    const auto snapshot = m_tasks;
    for (const QVariantMap &task : snapshot)
        if (kind.isEmpty() || task.value("kind").toString() == kind) pauseTask(task.value("id").toString());
}

void TransferManager::resumeAll(const QString &kind) {
    const auto snapshot = m_tasks;
    for (const QVariantMap &task : snapshot)
        if ((kind.isEmpty() || task.value("kind").toString() == kind) &&
            (task.value("status").toString() == "paused" || task.value("status").toString() == "interrupted"))
            resumeTask(task.value("id").toString());
}

void TransferManager::cancelAll(const QString &kind) {
    const auto snapshot = m_tasks;
    m_suppressQueueProcessing = true;
    for (const QVariantMap &task : snapshot)
        if ((kind.isEmpty() || task.value("kind").toString() == kind) && !terminalStatus(task.value("status").toString()))
            cancelTask(task.value("id").toString());
    m_suppressQueueProcessing = false;
    processQueues();
}

void TransferManager::removeFinishedTasks(const QString &kind) {
    for (int i = m_tasks.size() - 1; i >= 0; --i) {
        const QString status = m_tasks[i].value("status").toString();
        if ((kind.isEmpty() || m_tasks[i].value("kind").toString() == kind) &&
            (terminalStatus(status) || status == "failed" || status == "interrupted")) m_tasks.removeAt(i);
    }
    changed();
}
