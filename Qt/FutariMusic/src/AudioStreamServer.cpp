#include "AudioStreamServer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHostAddress>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>
#include <QThreadPool>
#include <QTimer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QUuid>
#include <QRunnable>

#include <algorithm>

struct AudioStreamServer::StreamRequest {
    QPointer<QTcpSocket> socket;
    QPointer<QNetworkReply> reply;
    QFile cachedFile;
    QString hash;
    QByteArray rangeHeader;
    ByteRange requestedRange;
    qint64 cacheWriteOffset = -1;
    qint64 cachedReadEnd = -1;
    qint64 upstreamBytesRead = 0;
    qint64 localBytesQueued = 0;
    quint64 session = 0;
    int retryCount = 0;
    int upstreamStatus = 0;
    bool hasRequestedRange = false;
    bool responseStarted = false;
    bool localResponse = false;
    bool headOnly = false;
    bool upstreamFinished = false;
};

namespace {
constexpr int kRangesSaveIntervalMs = 500;
constexpr int kRangesPublishIntervalMs = 80;
constexpr int kMaxInitialRequestRetries = 2;

bool isRetryableStatus(int status) { return status >= 500 && status < 600; }

QString contentTypeFor(const QString &format) {
    if (format == QStringLiteral("mp3")) return QStringLiteral("audio/mpeg");
    if (format == QStringLiteral("flac")) return QStringLiteral("audio/flac");
    if (format == QStringLiteral("ogg")) return QStringLiteral("audio/ogg");
    if (format == QStringLiteral("wav")) return QStringLiteral("audio/wav");
    if (format == QStringLiteral("m4a") || format == QStringLiteral("aac"))
        return QStringLiteral("audio/mp4");
    return QStringLiteral("application/octet-stream");
}

QByteArray statusText(int status) {
    switch (status) {
    case 200: return "OK";
    case 206: return "Partial Content";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 416: return "Range Not Satisfiable";
    case 502: return "Bad Gateway";
    default: return "Upstream Error";
    }
}

QByteArray responseHeaders(int status, const QByteArray &contentType, qint64 contentLength,
                           const QByteArray &contentRange = {}) {
    QByteArray headers = "HTTP/1.1 " + QByteArray::number(status) + " " + statusText(status) + "\r\n";
    headers += "Content-Type: " + contentType + "\r\n";
    headers += "Accept-Ranges: bytes\r\n";
    if (contentLength >= 0) headers += "Content-Length: " + QByteArray::number(contentLength) + "\r\n";
    if (!contentRange.isEmpty()) headers += "Content-Range: " + contentRange + "\r\n";
    headers += "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
    return headers;
}
}  // namespace

AudioStreamServer::AudioStreamServer(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &AudioStreamServer::acceptConnections);
    m_rangesSaveTimer.setSingleShot(true);
    connect(&m_rangesSaveTimer, &QTimer::timeout, this, &AudioStreamServer::saveCachedRanges);
    m_rangesPublishTimer.setSingleShot(true);
    connect(&m_rangesPublishTimer, &QTimer::timeout, this, &AudioStreamServer::bufferedRangesChanged);
}

AudioStreamServer::~AudioStreamServer() { stop(); }

QUrl AudioStreamServer::start(const QUrl &apiBase, const QString &authorizationToken,
                              const QString &hash, const QString &format, qint64 expectedSize,
                              const QString &cacheDirectory, bool persistCache) {
    stop();
    m_apiBase = apiBase;
    m_token = authorizationToken;
    m_hash = hash;
    m_format = format;
    m_totalBytes = qMax<qint64>(0, expectedSize);
    m_persistCache = persistCache;
    m_cacheFinalized = false;
    m_finalizationPending = false;
    m_routeToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ++m_session;

    QDir().mkpath(cacheDirectory);
    m_completedPath = QDir(cacheDirectory).filePath(hash + "." + format);
    m_partialPath = m_completedPath + ".part";
    m_rangesPath = m_partialPath + ".ranges.json";
    loadCachedRanges();

    if (!m_server.listen(QHostAddress::LocalHost, 0)) {
        emit sourceError(QStringLiteral("无法启动本地音频流"));
        return {};
    }
    QUrl streamUrl;
    streamUrl.setScheme(QStringLiteral("http"));
    streamUrl.setHost(m_server.serverAddress().toString());
    streamUrl.setPort(m_server.serverPort());
    streamUrl.setPath(QStringLiteral("/stream/") + hash + QLatin1Char('.') + format);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("session"), m_routeToken);
    streamUrl.setQuery(query);
    tryCompleteCache();
    publishRangeChanges();
    return streamUrl;
}

void AudioStreamServer::stop() {
    if (m_persistCache && !m_cacheFinalized && !m_partialPath.isEmpty()) saveCachedRanges();
    m_cacheWriteFile.close();
    m_cacheWriteFile.setFileName({});
    m_rangesSaveTimer.stop();
    m_rangesPublishTimer.stop();
    ++m_session;
    const QList<QNetworkReply *> replies = m_replies.values();
    for (QNetworkReply *reply : replies) {
        if (reply) reply->abort();
    }
    m_replies.clear();
    for (QTcpSocket *socket : std::as_const(m_sockets)) {
        if (!socket) continue;
        socket->abort();
        socket->deleteLater();
    }
    m_sockets.clear();
    m_requestBuffers.clear();
    m_server.close();
    m_ranges.clear();
    m_apiBase.clear();
    m_token.clear();
    m_hash.clear();
    m_format.clear();
    m_partialPath.clear();
    m_completedPath.clear();
    m_rangesPath.clear();
    m_routeToken.clear();
    m_totalBytes = 0;
    m_persistCache = false;
    m_cacheFinalized = false;
    m_finalizationPending = false;
    emit bufferedRangesChanged();
}

QVariantList AudioStreamServer::bufferedRanges() const {
    QVariantList result;
    if (m_totalBytes <= 0) return result;
    result.reserve(m_ranges.size());
    for (const ByteRange &range : m_ranges) {
        result.append(QVariantMap{{QStringLiteral("start"), static_cast<qreal>(range.start) / m_totalBytes},
                                  {QStringLiteral("end"), static_cast<qreal>(range.end + 1) / m_totalBytes}});
    }
    return result;
}

qreal AudioStreamServer::bufferProgress() const {
    if (m_totalBytes <= 0 || m_ranges.isEmpty()) return 0;
    return qBound<qreal>(0, static_cast<qreal>(m_ranges.constLast().end + 1) / m_totalBytes, 1);
}

void AudioStreamServer::acceptConnections() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        m_sockets.insert(socket);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { readRequest(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            m_sockets.remove(socket);
            m_requestBuffers.remove(socket);
            socket->deleteLater();
        });
        readRequest(socket);
    }
}

void AudioStreamServer::readRequest(QTcpSocket *socket) {
    if (!socket || !m_sockets.contains(socket)) return;
    QByteArray &buffer = m_requestBuffers[socket];
    buffer += socket->readAll();
    const qsizetype endOfHeaders = buffer.indexOf("\r\n\r\n");
    if (endOfHeaders < 0) {
        if (buffer.size() > 16 * 1024) socket->disconnectFromHost();
        return;
    }

    const QList<QByteArray> lines = buffer.left(endOfHeaders).split('\n');
    if (lines.isEmpty() || !socket->peerAddress().isLoopback()) {
        socket->write(responseHeaders(403, "text/plain", 0));
        socket->disconnectFromHost();
        return;
    }
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        socket->write(responseHeaders(400, "text/plain", 0));
        socket->disconnectFromHost();
        return;
    }
    const QByteArray method = requestLine.at(0).toUpper();
    const QUrl target = QUrl::fromEncoded(requestLine.at(1));
    const QString expectedPath = QStringLiteral("/stream/") + m_hash + QLatin1Char('.') + m_format;
    const QUrlQuery query(target);
    if ((method != "GET" && method != "HEAD") || target.path() != expectedPath ||
        query.queryItemValue(QStringLiteral("session")) != m_routeToken) {
        socket->write(responseHeaders(404, "text/plain", 0));
        socket->disconnectFromHost();
        return;
    }

    auto request = QSharedPointer<StreamRequest>::create();
    request->socket = socket;
    request->hash = m_hash;
    request->session = m_session;
    request->headOnly = method == "HEAD";
    QByteArray rangeHeader;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const qsizetype colon = line.indexOf(':');
        if (colon < 1) continue;
        const QByteArray name = line.left(colon).trimmed().toLower();
        const QByteArray value = line.mid(colon + 1).trimmed();
        if (name == "range") rangeHeader = value;
    }
    request->rangeHeader = rangeHeader;
    request->hasRequestedRange = parseClientRange(rangeHeader, &request->requestedRange);

    const bool cachedRangeReady = request->hasRequestedRange &&
        (!m_finalizationPending || m_cacheFinalized) &&
        isRangeCached(request->requestedRange.start, request->requestedRange.end) &&
        (!m_cacheWriteFile.isOpen() || m_cacheWriteFile.flush());
    if (cachedRangeReady) {
        const QString sourcePath = m_cacheFinalized ? m_completedPath : m_partialPath;
        request->cachedFile.setFileName(sourcePath);
        if (request->cachedFile.open(QIODevice::ReadOnly) &&
            request->cachedFile.seek(request->requestedRange.start)) {
            request->localResponse = true;
            request->cachedReadEnd = request->requestedRange.end;
            const qint64 length = request->cachedReadEnd - request->requestedRange.start + 1;
            const QByteArray contentRange = "bytes " + QByteArray::number(request->requestedRange.start) +
                "-" + QByteArray::number(request->cachedReadEnd) + "/" + QByteArray::number(m_totalBytes);
            socket->write(responseHeaders(206, contentTypeFor(m_format).toLatin1(), length, contentRange));
            connect(socket, &QTcpSocket::bytesWritten, this, [this, request] {
                pumpCachedResponse(request);
            });
            if (!request->headOnly) pumpCachedResponse(request);
            else socket->disconnectFromHost();
            return;
        }
    }

    startUpstream(request);
}

bool AudioStreamServer::parseClientRange(const QByteArray &header, ByteRange *range) const {
    if (!range || m_totalBytes <= 0 || !header.toLower().startsWith("bytes=")) return false;
    const QList<QByteArray> parts = header.mid(6).split(',');
    if (parts.size() != 1) return false;
    const QByteArray item = parts.first().trimmed();
    const qsizetype separator = item.indexOf('-');
    if (separator < 0) return false;
    bool startOk = false;
    bool endOk = false;
    const QByteArray startText = item.left(separator).trimmed();
    const QByteArray endText = item.mid(separator + 1).trimmed();
    if (startText.isEmpty()) {
        const qint64 suffix = endText.toLongLong(&endOk);
        if (!endOk || suffix <= 0) return false;
        range->start = qMax<qint64>(0, m_totalBytes - suffix);
        range->end = m_totalBytes - 1;
        return true;
    }
    range->start = startText.toLongLong(&startOk);
    if (!startOk || range->start < 0 || range->start >= m_totalBytes) return false;
    range->end = endText.isEmpty() ? m_totalBytes - 1 : endText.toLongLong(&endOk);
    if (!endText.isEmpty() && !endOk) return false;
    range->end = qMin(range->end, m_totalBytes - 1);
    return range->end >= range->start;
}

bool AudioStreamServer::parseContentRange(const QByteArray &header, ByteRange *range,
                                          qint64 *total) const {
    static const QRegularExpression pattern(QStringLiteral("^bytes\\s+(\\d+)-(\\d+)/(\\d+)$"),
                                            QRegularExpression::CaseInsensitiveOption);
    const auto match = pattern.match(QString::fromLatin1(header.trimmed()));
    if (!match.hasMatch()) return false;
    bool startOk = false;
    bool endOk = false;
    bool totalOk = false;
    range->start = match.captured(1).toLongLong(&startOk);
    range->end = match.captured(2).toLongLong(&endOk);
    *total = match.captured(3).toLongLong(&totalOk);
    return startOk && endOk && totalOk && range->start >= 0 && range->end >= range->start && *total > range->end;
}

bool AudioStreamServer::isRangeCached(qint64 start, qint64 end) const {
    for (const ByteRange &range : m_ranges)
        if (range.start <= start && range.end >= end) return true;
    return false;
}

void AudioStreamServer::startUpstream(const QSharedPointer<StreamRequest> &request) {
    if (!request || !request->socket) return;
    QUrl url = m_apiBase.resolved(QUrl(QStringLiteral("api/songs/") + m_hash + QStringLiteral("/file")));
    QNetworkRequest networkRequest(url);
    networkRequest.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    networkRequest.setTransferTimeout(120000);
    networkRequest.setRawHeader("Connection", "close");
    if (!request->rangeHeader.isEmpty()) networkRequest.setRawHeader("Range", request->rangeHeader);
    QNetworkReply *reply = request->headOnly ? m_network.head(networkRequest) : m_network.get(networkRequest);
    request->reply = reply;
    m_replies.insert(reply);
    reply->setReadBufferSize(512 * 1024);
    connect(reply, &QNetworkReply::metaDataChanged, this, [this, request] {
        const int status = request->reply
            ? request->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
            : 0;
        request->upstreamStatus = status;
        if (status >= 500 && status < 600) {
            qWarning().noquote() << "[audio stream] upstream returned HTTP error"
                                 << "hash=" << request->hash
                                 << "range=" << request->rangeHeader
                                 << "status=" << status
                                 << "contentRange=" << request->reply->rawHeader("Content-Range")
                                 << "retry=" << request->retryCount;
        }
        if (isRetryableStatus(status) && request->retryCount < kMaxInitialRequestRetries) return;
        beginResponse(request);
    });
    connect(reply, &QIODevice::readyRead, this, [this, request] { forwardAvailableData(request); });
    connect(request->socket, &QTcpSocket::bytesWritten, this, [this, request] {
        forwardAvailableData(request);
        finishUpstreamResponse(request);
    });
    connect(request->socket, &QTcpSocket::disconnected, this, [this, request] {
        if (!request->reply) return;
        QNetworkReply *reply = request->reply;
        qInfo().noquote() << "[audio stream] local media request closed with upstream active"
                          << "hash=" << request->hash
                          << "range=" << request->rangeHeader
                          << "status=" << request->upstreamStatus
                          << "responseStarted=" << request->responseStarted
                          << "upstreamBytesRead=" << request->upstreamBytesRead
                          << "localBytesQueued=" << request->localBytesQueued;
        reply->abort();
        reply->deleteLater();
        request->reply.clear();
        m_replies.remove(reply);
    });
    connect(reply, &QNetworkReply::finished, this, [this, request, reply] {
        m_replies.remove(reply);
        // FFmpeg closes a local range request when it seeks or replaces the
        // current read. That aborts the upstream reply intentionally; do not
        // turn the resulting OperationCanceledError into a playback error.
        if (!request->socket || request->socket->state() != QAbstractSocket::ConnectedState ||
            request->session != m_session) {
            reply->deleteLater();
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        request->upstreamStatus = status;
        const bool retryableFailure = isRetryableStatus(status) ||
            (status == 0 && reply->error() != QNetworkReply::NoError);
        if (!request->responseStarted && retryableFailure &&
            request->retryCount < kMaxInitialRequestRetries && request->socket &&
            request->socket->state() == QAbstractSocket::ConnectedState &&
            request->session == m_session) {
            ++request->retryCount;
            request->reply.clear();
            reply->deleteLater();
            QTimer::singleShot(100 * request->retryCount, this, [this, request] {
                if (request->session == m_session && request->socket &&
                    request->socket->state() == QAbstractSocket::ConnectedState)
                    startUpstream(request);
            });
            return;
        }
        request->upstreamFinished = true;
        beginResponse(request);
        forwardAvailableData(request);
        if (status >= 500 || reply->error() != QNetworkReply::NoError) {
            qWarning().noquote() << "[audio stream] upstream request finished with failure"
                                 << "hash=" << request->hash
                                 << "range=" << request->rangeHeader
                                 << "status=" << status
                                 << "retry=" << request->retryCount
                                 << "error=" << reply->errorString()
                                 << "upstreamBytesRead=" << request->upstreamBytesRead
                                 << "localBytesQueued=" << request->localBytesQueued;
        }
        if (!request->responseStarted && request->socket) {
            qWarning().noquote() << "[audio stream] upstream returned no response headers after retries"
                                 << "hash=" << request->hash
                                 << "status=" << status
                                 << "range=" << request->rangeHeader
                                 << "error=" << reply->errorString();
            request->socket->write(responseHeaders(502, "text/plain", 0));
        }
        if (reply->error() != QNetworkReply::NoError && request->session == m_session)
            emit sourceError(reply->errorString());
        finishUpstreamResponse(request);
    });
}

void AudioStreamServer::beginResponse(const QSharedPointer<StreamRequest> &request) {
    if (!request || request->responseStarted || !request->reply || !request->socket) return;
    QNetworkReply *reply = request->reply;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status <= 0) return;
    if (isRetryableStatus(status) && request->retryCount < kMaxInitialRequestRetries) return;
    const QByteArray contentRangeHeader = reply->rawHeader("Content-Range");
    ByteRange actualRange;
    qint64 actualTotal = 0;
    qint64 actualLength = -1;
    if (status == 206 && parseContentRange(contentRangeHeader, &actualRange, &actualTotal)) {
        request->cacheWriteOffset = actualRange.start;
        actualLength = actualRange.end - actualRange.start + 1;
        if (actualTotal > 0) m_totalBytes = actualTotal;
    } else if (status == 200) {
        request->cacheWriteOffset = 0;
        if (m_totalBytes <= 0) {
            const qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
            if (contentLength > 0) m_totalBytes = contentLength;
        }
    } else {
        request->cacheWriteOffset = -1;
    }

    QByteArray contentType = reply->rawHeader("Content-Type");
    if (contentType.isEmpty()) contentType = contentTypeFor(m_format).toLatin1();
    qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    if (contentLength <= 0 && status == 206) contentLength = actualLength;
    if (contentLength <= 0 && status == 200 && m_totalBytes > 0) contentLength = m_totalBytes;
    request->socket->write(responseHeaders(status, contentType, contentLength, contentRangeHeader));
    request->responseStarted = true;
}

void AudioStreamServer::forwardAvailableData(const QSharedPointer<StreamRequest> &request) {
    if (!request || !request->reply || !request->socket || request->session != m_session) return;
    beginResponse(request);
    if (!request->responseStarted || request->headOnly || request->socket->bytesToWrite() > 512 * 1024) return;
    const QByteArray bytes = request->reply->readAll();
    if (bytes.isEmpty()) return;
    request->upstreamBytesRead += bytes.size();
    if (request->cacheWriteOffset >= 0 && !m_cacheFinalized && !m_finalizationPending)
        recordCachedBytes(request->cacheWriteOffset, bytes);
    request->cacheWriteOffset += bytes.size();
    const qint64 queued = request->socket->write(bytes);
    if (queued > 0) request->localBytesQueued += queued;
}

void AudioStreamServer::finishUpstreamResponse(const QSharedPointer<StreamRequest> &request) {
    if (!request || !request->upstreamFinished || !request->socket ||
        (request->reply && request->reply->isOpen() && request->reply->bytesAvailable() > 0) ||
        request->socket->bytesToWrite() > 0)
        return;
    request->socket->disconnectFromHost();
    if (request->reply) {
        request->reply->deleteLater();
        request->reply.clear();
    }
}

void AudioStreamServer::pumpCachedResponse(const QSharedPointer<StreamRequest> &request) {
    if (!request || !request->socket || !request->localResponse || request->headOnly) return;
    while (request->socket->bytesToWrite() < 128 * 1024 && request->cachedFile.pos() <= request->cachedReadEnd) {
        const qint64 remaining = request->cachedReadEnd - request->cachedFile.pos() + 1;
        const QByteArray bytes = request->cachedFile.read(qMin<qint64>(64 * 1024, remaining));
        if (bytes.isEmpty()) break;
        request->socket->write(bytes);
    }
    if (request->cachedFile.pos() > request->cachedReadEnd) request->socket->disconnectFromHost();
}

void AudioStreamServer::recordCachedBytes(qint64 start, const QByteArray &bytes) {
    if (bytes.isEmpty() || m_partialPath.isEmpty()) return;
    if (!m_cacheWriteFile.isOpen()) {
        m_cacheWriteFile.setFileName(m_partialPath);
        if (!m_cacheWriteFile.open(QIODevice::ReadWrite)) {
            emit sourceError(QStringLiteral("写入音乐缓存失败"));
            return;
        }
    }
    if (!m_cacheWriteFile.seek(start) || m_cacheWriteFile.write(bytes) != bytes.size()) {
        emit sourceError(QStringLiteral("写入音乐缓存失败"));
        return;
    }
    mergeRange(start, start + bytes.size() - 1);
    publishRangeChanges();
    if (m_persistCache) scheduleRangesSave();
    tryCompleteCache();
}

void AudioStreamServer::mergeRange(qint64 start, qint64 end) {
    if (m_totalBytes > 0) {
        start = qBound<qint64>(0, start, m_totalBytes - 1);
        end = qBound<qint64>(start, end, m_totalBytes - 1);
    }
    ByteRange merged{start, end};
    QList<ByteRange> result;
    bool inserted = false;
    for (const ByteRange &range : std::as_const(m_ranges)) {
        if (range.end + 1 < merged.start) {
            result.append(range);
        } else if (merged.end + 1 < range.start) {
            if (!inserted) { result.append(merged); inserted = true; }
            result.append(range);
        } else {
            merged.start = qMin(merged.start, range.start);
            merged.end = qMax(merged.end, range.end);
        }
    }
    if (!inserted) result.append(merged);
    m_ranges = result;
}

void AudioStreamServer::loadCachedRanges() {
    m_ranges.clear();
    if (!m_persistCache || !QFile::exists(m_partialPath)) return;
    if (m_totalBytes > 0 && QFileInfo(m_partialPath).size() > m_totalBytes) {
        QFile::remove(m_partialPath);
        QFile::remove(m_rangesPath);
        return;
    }
    QFile metadata(m_rangesPath);
    if (metadata.open(QIODevice::ReadOnly)) {
        const QJsonObject object = QJsonDocument::fromJson(metadata.readAll()).object();
        const qint64 storedSize = object.value(QStringLiteral("size")).toVariant().toLongLong();
        if (m_totalBytes > 0 && storedSize > 0 && storedSize != m_totalBytes) {
            metadata.close();
            QFile::remove(m_partialPath);
            QFile::remove(m_rangesPath);
            return;
        }
        if (m_totalBytes <= 0) m_totalBytes = storedSize;
        const QJsonArray ranges = object.value(QStringLiteral("ranges")).toArray();
        for (const QJsonValue &value : ranges) {
            const QJsonArray pair = value.toArray();
            if (pair.size() != 2) continue;
            bool startOk = false;
            bool endOk = false;
            const qint64 start = pair.at(0).toVariant().toLongLong(&startOk);
            const qint64 end = pair.at(1).toVariant().toLongLong(&endOk);
            if (startOk && endOk && start >= 0 && end >= start &&
                (m_totalBytes <= 0 || end < m_totalBytes)) mergeRange(start, end);
        }
        metadata.close();
        return;
    }

    // A sparse .part file has the size of its highest written offset. Without the
    // sidecar we cannot prove which bytes exist, so never serve its holes as audio.
    QFile::remove(m_partialPath);
}

void AudioStreamServer::scheduleRangesSave() {
    // Coalesce many network chunks into one metadata write.
    if (!m_rangesSaveTimer.isActive()) m_rangesSaveTimer.start(kRangesSaveIntervalMs);
}

void AudioStreamServer::saveCachedRanges() {
    if (!m_persistCache || m_rangesPath.isEmpty()) return;
    if (m_cacheWriteFile.isOpen() && !m_cacheWriteFile.flush()) return;
    if (m_cacheFinalized) {
        QFile::remove(m_rangesPath);
        return;
    }
    if (m_ranges.isEmpty()) {
        QFile::remove(m_rangesPath);
        return;
    }
    QJsonArray ranges;
    for (const ByteRange &range : std::as_const(m_ranges))
        ranges.append(QJsonArray{static_cast<double>(range.start), static_cast<double>(range.end)});
    QSaveFile metadata(m_rangesPath);
    if (metadata.open(QIODevice::WriteOnly)) {
        metadata.write(QJsonDocument(QJsonObject{{QStringLiteral("size"), m_totalBytes},
                                                 {QStringLiteral("ranges"), ranges}}).toJson(QJsonDocument::Compact));
        metadata.commit();
    }
}

void AudioStreamServer::tryCompleteCache() {
    if (!m_persistCache || m_cacheFinalized || m_finalizationPending || m_totalBytes <= 0 ||
        m_ranges.size() != 1 || m_ranges.first().start != 0 || m_ranges.first().end + 1 != m_totalBytes)
        return;
    m_finalizationPending = true;
    saveCachedRanges();
    m_cacheWriteFile.close();
    const QString partialPath = m_partialPath;
    const QString completedPath = m_completedPath;
    const QString rangesPath = m_rangesPath;
    const QByteArray expectedHash = m_hash.toLatin1();
    const quint64 session = m_session;
    QPointer<AudioStreamServer> guard(this);
    QThreadPool::globalInstance()->start(QRunnable::create(
        [guard, partialPath, completedPath, rangesPath, expectedHash, session] {
            QCryptographicHash digest(QCryptographicHash::Sha256);
            QFile file(partialPath);
            bool valid = file.open(QIODevice::ReadOnly);
            while (valid && !file.atEnd()) {
                const QByteArray chunk = file.read(256 * 1024);
                if (chunk.isEmpty() && !file.atEnd()) { valid = false; break; }
                digest.addData(chunk);
            }
            valid = valid && digest.result().toHex() == expectedHash;
            if (!guard) return;
            QMetaObject::invokeMethod(guard.data(), [guard, session, valid, partialPath,
                                                       completedPath, rangesPath] {
                if (!guard || guard->m_session != session) return;
                guard->m_finalizationPending = false;
                if (valid) {
                    QFile::remove(completedPath);
                    if (QFile::rename(partialPath, completedPath)) {
                        QFile::remove(rangesPath);
                        guard->m_cacheFinalized = true;
                        guard->m_ranges = {{0, guard->m_totalBytes - 1}};
                        guard->publishRangeChanges();
                        emit guard->cacheCompleted();
                    } else
                        QTimer::singleShot(500, guard.data(), [guard] {
                            if (guard) guard->tryCompleteCache();
                        });
                } else {
                    QFile::remove(partialPath);
                    QFile::remove(rangesPath);
                    guard->m_ranges.clear();
                    guard->publishRangeChanges();
                }
            }, Qt::QueuedConnection);
        }));
}

void AudioStreamServer::publishRangeChanges() {
    // One UI update per frame is enough; raw chunks can arrive hundreds of times per second.
    if (!m_rangesPublishTimer.isActive()) m_rangesPublishTimer.start(kRangesPublishIntervalMs);
}

