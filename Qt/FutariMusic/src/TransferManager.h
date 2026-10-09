#ifndef FUTARI_TRANSFER_MANAGER_H
#define FUTARI_TRANSFER_MANAGER_H

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
    Q_DISABLE_COPY(TransferManager)
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
    // 按服务器和用户持久化的任务快照，是 UI 状态的唯一来源。
    // waiting → uploading/downloading → success/failed；暂停保留断点，取消清理在途文件。
    QVector<QVariantMap> m_tasks;
    // 每个任务的在途请求；QPointer 在网络对象释放后自动置空。
    QHash<QString, QPointer<QNetworkReply>> m_replies;
    // 速率采样和落盘节流时间；仅用于展示/持久化，不决定任务完成。
    QHash<QString, qint64> m_lastProgressBytes;
    QHash<QString, qint64> m_lastProgressTime;
    QHash<QString, qint64> m_lastPersistTime;
    // 取消请求后等 finished 关闭文件，再清理断点，避免删除仍打开的文件。
    QHash<QString, QString> m_cancelledDownloadPaths;
    // 批量暂停/切换账号期间禁止 abort 回调启动下一任务。
    bool m_suppressQueueProcessing = false;
    // 上传串行调度锁；空字符串表示可启动下一个 waiting 任务。
    QString m_uploadActiveId;
};

#endif  // FUTARI_TRANSFER_MANAGER_H
