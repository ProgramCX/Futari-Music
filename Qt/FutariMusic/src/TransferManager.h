#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QVector>
#include <functional>
#include <memory>

struct DownloadState;

class TransferManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY tasksChanged)
public:
    explicit TransferManager(QObject *parent = nullptr);
    QVariantList tasks() const;
    void setContext(const QUrl &baseUrl, const QString &token, qint64 userId);
    Q_INVOKABLE QString enqueueUpload(const QVariantMap &metadata);
    Q_INVOKABLE QString enqueueDownload(const QVariantMap &song, const QString &downloadDirectory);
    Q_INVOKABLE void pauseTask(const QString &taskId);
    Q_INVOKABLE void resumeTask(const QString &taskId);
    Q_INVOKABLE void cancelTask(const QString &taskId);
    Q_INVOKABLE void retryTask(const QString &taskId);
    Q_INVOKABLE void pauseAll(const QString &kind);
    Q_INVOKABLE void resumeAll(const QString &kind);
    Q_INVOKABLE void cancelAll(const QString &kind);
    Q_INVOKABLE void removeFinishedTasks(const QString &kind);
signals:
    void tasksChanged();
    void taskSucceeded(const QString &taskId, const QVariantMap &song, const QString &kind);
    void unauthorized();
private:
    int indexOf(const QString &taskId) const;
    void persist();
    void loadForCurrentUser();
    void changed(bool save = true);
    void processQueues();
    void startUpload(const QString &taskId);
    void startDownload(const QString &taskId);
    void finishUpload(const QString &taskId, QNetworkReply *reply);
    void finishDownload(const QString &taskId, QNetworkReply *reply,
                        const std::shared_ptr<DownloadState> &state);
    void updateProgress(const QString &taskId, qint64 transferred, qint64 total);
    void markInterrupted(const QString &taskId, const QString &message);
    QNetworkRequest makeRequest(const QString &path) const;
    QNetworkAccessManager m_network;
    QSettings m_settings;
    QUrl m_baseUrl;
    QString m_token;
    qint64 m_userId = 0;
    QVector<QVariantMap> m_tasks;
    QHash<QString, QPointer<QNetworkReply>> m_replies;
    QHash<QString, qint64> m_lastProgressBytes;
    QHash<QString, qint64> m_lastProgressTime;
    QHash<QString, qint64> m_lastPersistTime;
    QHash<QString, QString> m_cancelledDownloadPaths;
    bool m_suppressQueueProcessing = false;
    QString m_uploadActiveId;
};
