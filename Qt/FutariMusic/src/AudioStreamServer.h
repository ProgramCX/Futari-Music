#ifndef FUTARI_AUDIO_STREAM_SERVER_H
#define FUTARI_AUDIO_STREAM_SERVER_H

#include <QHash>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTcpServer>
#include <QVariantList>

class QNetworkReply;
class QTcpSocket;

// Serves authenticated song ranges to Qt Multimedia and records the exact cached byte intervals.
class AudioStreamServer final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AudioStreamServer)
public:
    explicit AudioStreamServer(QObject *parent = nullptr);
    ~AudioStreamServer() override;

    QUrl start(const QUrl &apiBase, const QString &authorizationToken,
               const QString &hash, const QString &format, qint64 expectedSize,
               const QString &cacheDirectory, bool persistCache);
    void stop();

    QVariantList bufferedRanges() const;
    qreal bufferProgress() const;
    QString partialPath() const { return m_partialPath; }
    QString completedPath() const { return m_completedPath; }
    QString rangesPath() const { return m_rangesPath; }

signals:
    void bufferedRangesChanged();
    void cacheCompleted();
    void sourceError(const QString &message);

private:
    struct ByteRange { qint64 start = 0; qint64 end = -1; };
    struct StreamRequest;

    void acceptConnections();
    void readRequest(QTcpSocket *socket);
    void startUpstream(const QSharedPointer<StreamRequest> &request);
    void beginResponse(const QSharedPointer<StreamRequest> &request);
    void forwardAvailableData(const QSharedPointer<StreamRequest> &request);
    void finishUpstreamResponse(const QSharedPointer<StreamRequest> &request);
    void pumpCachedResponse(const QSharedPointer<StreamRequest> &request);
    bool isRangeCached(qint64 start, qint64 end) const;
    bool parseClientRange(const QByteArray &header, ByteRange *range) const;
    bool parseContentRange(const QByteArray &header, ByteRange *range, qint64 *total) const;
    void recordCachedBytes(qint64 start, const QByteArray &bytes);
    void mergeRange(qint64 start, qint64 end);
    void loadCachedRanges();
    void saveCachedRanges();
    void scheduleRangesSave();
    void tryCompleteCache();
    void publishRangeChanges();

    QTcpServer m_server;
    QNetworkAccessManager m_network;
    // Each player request is short-lived; stale sockets/replies are aborted on source changes.
    QHash<QTcpSocket *, QByteArray> m_requestBuffers;
    QSet<QTcpSocket *> m_sockets;
    QSet<QNetworkReply *> m_replies;
    // Sorted, merged, inclusive byte intervals; file length alone cannot prove sparse cache coverage.
    QList<ByteRange> m_ranges;
    QUrl m_apiBase;
    QString m_token;
    QString m_hash;
    QString m_format;
    QString m_partialPath;
    QString m_completedPath;
    QString m_rangesPath;
    QString m_routeToken;
    // Server metadata is authoritative when the song response omitted the expected size.
    qint64 m_totalBytes = 0;
    bool m_persistCache = false;
    bool m_cacheFinalized = false;
    // Freeze cache writes while the complete sparse file is checked off the UI thread.
    bool m_finalizationPending = false;
    // Incremented for each source so late network callbacks cannot update another song's state.
    quint64 m_session = 0;
};

#endif  // FUTARI_AUDIO_STREAM_SERVER_H
