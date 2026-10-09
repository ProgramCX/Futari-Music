#ifndef FUTARI_MUSIC_CLIENT_UPDATE_MANAGER_H_
#define FUTARI_MUSIC_CLIENT_UPDATE_MANAGER_H_

#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>
#include <memory>

class QProcess;

// Handles public update metadata, verified downloads, and platform installation handoff.
class ClientUpdateManager final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(ClientUpdateManager)
    Q_PROPERTY(QString serverVersion READ serverVersion NOTIFY updateInfoChanged)
    Q_PROPERTY(QString currentClientVersion READ currentClientVersion CONSTANT)
    Q_PROPERTY(QString currentReleaseTag READ currentReleaseTag CONSTANT)
    Q_PROPERTY(QString latestClientVersion READ latestClientVersion NOTIFY updateInfoChanged)
    Q_PROPERTY(QString releaseTag READ releaseTag NOTIFY updateInfoChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateInfoChanged)
    Q_PROPERTY(bool autoSilentUpdate READ autoSilentUpdate WRITE setAutoSilentUpdate NOTIFY
                   autoSilentUpdateChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadStateChanged)
    Q_PROPERTY(bool installing READ installing NOTIFY downloadStateChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(bool progressIndeterminate READ progressIndeterminate NOTIFY downloadProgressChanged)
public:
    explicit ClientUpdateManager(QObject* parent = nullptr);

    QString serverVersion() const { return m_serverVersion; }
    QString currentClientVersion() const;
    QString currentReleaseTag() const;
    QString latestClientVersion() const { return m_latestClientVersion; }
    QString releaseTag() const { return m_releaseTag; }
    QString statusMessage() const { return m_statusMessage; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool autoSilentUpdate() const { return m_settings.value("autoSilentUpdate", true).toBool(); }
    void setAutoSilentUpdate(bool enabled);
    bool checking() const { return m_checking; }
    bool downloading() const { return m_downloading; }
    bool installing() const { return m_installing; }
    int downloadProgress() const { return m_downloadProgress; }
    bool progressIndeterminate() const { return m_progressIndeterminate; }

    void setServerUrl(const QString& url);
    Q_INVOKABLE void checkForUpdates(bool startup_check = false);
    Q_INVOKABLE void startUpdate();
    Q_INVOKABLE void cancelDownload();
    void restartAfterQuit();

signals:
    void updateInfoChanged();
    void autoSilentUpdateChanged();
    void checkingChanged();
    void downloadStateChanged();
    void downloadProgressChanged();
    void statusMessageChanged();
    void updatePromptRequested();
    void updateStarted(const QString& release_tag);
    void updateSucceeded(const QString& release_tag);
    void updateFailed(const QString& message);
    void requestApplicationClose();

private:
    void handleCheckReply(QNetworkReply* reply, bool startup_check);
    void handleUpdateCheckResult(bool startup_check);
    void scheduleStartupRetry();
    bool isNewerCompatibleRelease() const;
    bool hasSupportedInstallerFile() const;
    bool hasTrustedDownloadUrl() const;
    void startDownload();
    void writeDownloadChunk();
    void finishDownload();
    bool verifyDownloadedFile();
    void failDownload(const QString& message, bool notify = true);
    void installDownloadedUpdate();
    bool startWindowsInstaller();
    bool startUbuntuInstaller();
    void setStatusMessage(const QString& message);
    void setChecking(bool checking);
    void setDownloadProgress(qint64 received, qint64 total);
    static QString currentPlatform();
    static bool isNewerClientVersion(const QString& candidate, const QString& current);
    static QString serverCompatibilityLine(const QString& version);

    QNetworkAccessManager m_network;
    QSettings m_settings;
    QUrl m_serverUrl;
    QPointer<QNetworkReply> m_checkReply;
    QPointer<QNetworkReply> m_downloadReply;
    std::unique_ptr<QSaveFile> m_downloadFile;
    QProcess* m_installerProcess = nullptr;
    QCryptographicHash m_downloadHash{QCryptographicHash::Sha256};
    QString m_serverVersion;
    QString m_latestClientVersion;
    QString m_releaseTag;
    QString m_fileName;
    QString m_downloadUrl;
    QString m_expectedSha256;
    QString m_downloadPath;
    QString m_statusMessage;
    QString m_downloadError;
    qint64 m_receivedBytes = 0;
    qint64 m_totalBytes = -1;
    int m_downloadProgress = 0;
    bool m_updateAvailable = false;
    // 下载状态按阶段互斥：下载完成并校验通过后才进入安装，取消只作用于活动下载。
    bool m_checking = false;
    bool m_downloading = false;
    bool m_installing = false;
    bool m_progressIndeterminate = true;
    bool m_cancelRequested = false;
    bool m_restartAfterQuit = false;
    // 服务端刚启动时可能还在缓存 GitHub 安装包；有限重试避免客户端错过启动更新。
    bool m_startupRetryPending = false;
    int m_startupRetryAttempts = 0;
    quint64 m_serverUrlGeneration = 0;
};

#endif  // FUTARI_MUSIC_CLIENT_UPDATE_MANAGER_H_
