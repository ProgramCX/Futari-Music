#include "ClientUpdateManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QUrlQuery>
#include <algorithm>

namespace {
constexpr char kClientVersion[] = FUTARI_CLIENT_VERSION;
constexpr char kServerCompatibilityVersion[] = FUTARI_SERVER_COMPAT_VERSION;
#ifndef FUTARI_WINDOWS_INSTALL_ARGUMENTS
#define FUTARI_WINDOWS_INSTALL_ARGUMENTS "/S"
#endif
constexpr char kWindowsInstallerArguments[] = FUTARI_WINDOWS_INSTALL_ARGUMENTS;
constexpr int kRequestTimeoutMs = 30'000;
constexpr int kStartupRetryDelayMs = 10'000;
constexpr int kMaximumStartupRetries = 6;
}  // namespace

ClientUpdateManager::ClientUpdateManager(QObject* parent)
    : QObject(parent),
      m_settings(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral("Futari"),
                 QStringLiteral("FutariMusic")) {}

QString ClientUpdateManager::currentClientVersion() const {
    return QString::fromLatin1(kClientVersion);
}

QString ClientUpdateManager::currentReleaseTag() const {
    const QString compatibility_line =
        serverCompatibilityLine(QString::fromLatin1(kServerCompatibilityVersion));
    return QStringLiteral("%1-%2").arg(compatibility_line, currentClientVersion());
}

void ClientUpdateManager::setAutoSilentUpdate(bool enabled) {
    if (autoSilentUpdate() == enabled) return;
    m_settings.setValue(QStringLiteral("autoSilentUpdate"), enabled);
    emit autoSilentUpdateChanged();
}

void ClientUpdateManager::setServerUrl(const QString& url) {
    QUrl next_url(url);
    if (!next_url.path().endsWith('/')) next_url.setPath(next_url.path() + '/');
    if (m_serverUrl == next_url) return;
    ++m_serverUrlGeneration;
    m_startupRetryPending = false;
    m_startupRetryAttempts = 0;
    m_serverUrl = next_url;
    if (m_downloadReply) {
        // 服务端切换后丢弃旧源的下载，避免把旧服务器的包安装到新会话中。
        m_cancelRequested = true;
        m_downloadReply->abort();
    }
    m_serverVersion.clear();
    m_latestClientVersion.clear();
    m_releaseTag.clear();
    m_fileName.clear();
    m_downloadUrl.clear();
    m_expectedSha256.clear();
    m_updateAvailable = false;
    emit updateInfoChanged();
    if (m_checkReply) {
        QNetworkReply* stale_reply = m_checkReply;
        m_checkReply = nullptr;
        stale_reply->abort();
    }
    setChecking(false);
}

void ClientUpdateManager::checkForUpdates(bool startup_check) {
    if (m_checking || m_downloading || !m_serverUrl.isValid() || m_serverUrl.host().isEmpty())
        return;
    const QString platform = currentPlatform();
    if (platform.isEmpty()) {
        setStatusMessage(tr("此系统或处理器架构暂不支持自动更新。"));
        return;
    }
    QUrl url = m_serverUrl.resolved(QUrl(QStringLiteral("api/updates")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("platform"), platform);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(kRequestTimeoutMs);
    setStatusMessage(tr("正在检查更新…"));
    setChecking(true);
    QNetworkReply* reply = m_network.get(request);
    m_checkReply = reply;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, startup_check] { handleCheckReply(reply, startup_check); });
}

void ClientUpdateManager::handleCheckReply(QNetworkReply* reply, bool startup_check) {
    if (m_checkReply != reply) {
        reply->deleteLater();
        return;
    }
    m_checkReply = nullptr;
    setChecking(false);
    const QByteArray response_bytes = reply->readAll();
    const int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError network_error = reply->error();
    const QString network_message = reply->errorString();
    reply->deleteLater();
    if (network_error != QNetworkReply::NoError || status_code < 200 || status_code >= 300) {
        setStatusMessage(tr("检查更新失败：%1").arg(network_message));
        if (startup_check) scheduleStartupRetry();
        return;
    }
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(response_bytes, &parse_error);
    const QJsonObject envelope = document.object();
    const QJsonObject data = envelope.value(QStringLiteral("data")).toObject();
    if (parse_error.error != QJsonParseError::NoError ||
        envelope.value(QStringLiteral("code")).toInt(-1) != 0) {
        setStatusMessage(tr("服务器返回的更新信息无效。"));
        if (startup_check) scheduleStartupRetry();
        return;
    }
    m_serverVersion = data.value(QStringLiteral("serverVersion")).toString();
    m_latestClientVersion = data.value(QStringLiteral("clientVersion")).toString();
    m_releaseTag = data.value(QStringLiteral("releaseTag")).toString();
    m_fileName = data.value(QStringLiteral("fileName")).toString();
    m_expectedSha256 = data.value(QStringLiteral("sha256")).toString().toLower();
    m_downloadUrl = data.value(QStringLiteral("downloadUrl")).toString();
    m_updateAvailable = !m_releaseTag.isEmpty() && isNewerCompatibleRelease();
    emit updateInfoChanged();
    handleUpdateCheckResult(startup_check);
}

void ClientUpdateManager::handleUpdateCheckResult(bool startup_check) {
    if (m_releaseTag.isEmpty()) {
        if (startup_check && m_startupRetryAttempts < kMaximumStartupRetries) {
            setStatusMessage(
                tr("服务器 %1 的更新包尚未就绪，稍后重试。")
                    .arg(m_serverVersion.isEmpty() ? tr("当前版本") : m_serverVersion));
            scheduleStartupRetry();
        } else {
            setStatusMessage(
                tr("服务器 %1 尚未发布适用于此平台的客户端。")
                    .arg(m_serverVersion.isEmpty() ? tr("当前版本") : m_serverVersion));
        }
        return;
    }
    m_startupRetryAttempts = 0;
    m_startupRetryPending = false;
    if (!m_updateAvailable) {
        setStatusMessage(tr("客户端已是兼容的最新版本。"));
        return;
    }
    setStatusMessage(tr("发现兼容更新：%1").arg(m_releaseTag));
    if (startup_check && autoSilentUpdate()) {
        startDownload();
    } else {
        emit updatePromptRequested();
    }
}

void ClientUpdateManager::scheduleStartupRetry() {
    if (m_startupRetryPending || m_startupRetryAttempts >= kMaximumStartupRetries) return;
    m_startupRetryPending = true;
    ++m_startupRetryAttempts;
    const quint64 generation = m_serverUrlGeneration;
    QTimer::singleShot(kStartupRetryDelayMs, this, [this, generation] {
        if (!m_startupRetryPending || generation != m_serverUrlGeneration) return;
        m_startupRetryPending = false;
        if (!m_serverUrl.isValid() || m_downloading || m_installing) return;
        checkForUpdates(true);
    });
}

bool ClientUpdateManager::isNewerCompatibleRelease() const {
    const QString local_line =
        serverCompatibilityLine(QString::fromLatin1(kServerCompatibilityVersion));
    const QString target_server = m_releaseTag.section('-', 0, 0);
    const QString target_line = serverCompatibilityLine(target_server);
    if (local_line.isEmpty() || target_line.isEmpty()) return true;
    if (local_line != target_line) return true;
    return isNewerClientVersion(m_latestClientVersion, currentClientVersion());
}

void ClientUpdateManager::startUpdate() {
    if (!m_updateAvailable) return;
    startDownload();
}

void ClientUpdateManager::startDownload() {
    if (m_downloading || m_installing || m_downloadUrl.isEmpty()) return;
    const QString safe_file_name = QFileInfo(m_fileName).fileName();
    if (safe_file_name.isEmpty() || safe_file_name != m_fileName || !hasSupportedInstallerFile()) {
        failDownload(tr("服务器提供了无效或不受支持的更新文件名。"));
        return;
    }
    if (!hasTrustedDownloadUrl()) {
        failDownload(tr("服务器提供了无效的更新下载地址。"));
        return;
    }
    const QString temporary_directory =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QStringLiteral("/FutariMusicUpdates");
    if (!QDir().mkpath(temporary_directory)) {
        failDownload(tr("无法创建客户端更新临时目录。"));
        return;
    }
    m_downloadPath = QDir(temporary_directory).filePath(safe_file_name);
    m_downloadFile = std::make_unique<QSaveFile>(m_downloadPath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        m_downloadFile.reset();
        failDownload(tr("无法保存客户端更新文件。"));
        return;
    }
    m_downloadHash.reset();
    m_receivedBytes = 0;
    m_totalBytes = -1;
    m_downloadProgress = 0;
    m_progressIndeterminate = true;
    m_cancelRequested = false;
    m_downloadError.clear();
    m_downloading = true;
    m_installing = false;
    emit downloadStateChanged();
    emit downloadProgressChanged();
    setStatusMessage(tr("正在下载客户端更新…"));
    emit updateStarted(m_releaseTag);
    QUrl url = m_serverUrl.resolved(QUrl(m_downloadUrl));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/octet-stream");
    request.setTransferTimeout(0);
    QNetworkReply* reply = m_network.get(request);
    m_downloadReply = reply;
    connect(reply, &QIODevice::readyRead, this, &ClientUpdateManager::writeDownloadChunk);
    connect(reply, &QNetworkReply::downloadProgress, this,
            &ClientUpdateManager::setDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, &ClientUpdateManager::finishDownload);
}

bool ClientUpdateManager::hasSupportedInstallerFile() const {
    const QString file_name = m_fileName.toLower();
#if defined(Q_OS_WIN)
    return file_name.endsWith(QStringLiteral(".exe"));
#elif defined(Q_OS_LINUX)
    return file_name.endsWith(QStringLiteral(".deb"));
#else
    return false;
#endif
}

bool ClientUpdateManager::hasTrustedDownloadUrl() const {
    const QUrl url(m_downloadUrl);
    return url.isValid() && url.isRelative() && url.scheme().isEmpty() && url.host().isEmpty() &&
           url.path() == QStringLiteral("api/updates/download");
}

void ClientUpdateManager::writeDownloadChunk() {
    if (!m_downloadReply || !m_downloadFile) return;
    const QByteArray chunk = m_downloadReply->readAll();
    if (m_downloadFile->write(chunk) != chunk.size()) {
        m_downloadError = tr("更新文件写入失败，请检查临时目录空间。");
        m_downloadReply->abort();
        return;
    }
    m_downloadHash.addData(chunk);
}

void ClientUpdateManager::setDownloadProgress(qint64 received, qint64 total) {
    m_receivedBytes = received;
    m_totalBytes = total;
    m_progressIndeterminate = total <= 0;
    m_downloadProgress =
        total > 0 ? static_cast<int>(std::clamp<qint64>(received * 100 / total, 0, 100)) : 0;
    emit downloadProgressChanged();
}

void ClientUpdateManager::finishDownload() {
    QNetworkReply* reply = m_downloadReply;
    if (!reply) return;
    writeDownloadChunk();
    m_downloadReply = nullptr;
    const int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError network_error = reply->error();
    const QString network_message = reply->errorString();
    reply->deleteLater();
    if (m_cancelRequested) {
        if (m_downloadFile) m_downloadFile->cancelWriting();
        m_downloadFile.reset();
        m_downloading = false;
        emit downloadStateChanged();
        setStatusMessage(tr("已取消更新下载。"));
        return;
    }
    if (!m_downloadError.isEmpty()) return failDownload(m_downloadError);
    if (network_error != QNetworkReply::NoError || status_code < 200 || status_code >= 300) {
        return failDownload(tr("更新下载失败：%1").arg(network_message));
    }
    if (!verifyDownloadedFile()) return;
    installDownloadedUpdate();
}

bool ClientUpdateManager::verifyDownloadedFile() {
    const QByteArray actual_digest = m_downloadHash.result().toHex();
    if (!m_expectedSha256.isEmpty() && actual_digest != m_expectedSha256.toLatin1()) {
        failDownload(tr("更新文件校验失败，已取消安装。"));
        return false;
    }
    if (!m_downloadFile || !m_downloadFile->commit()) {
        failDownload(tr("更新文件保存失败。"));
        return false;
    }
    m_downloadFile.reset();
    m_downloading = false;
    m_downloadProgress = 100;
    m_progressIndeterminate = false;
    emit downloadStateChanged();
    emit downloadProgressChanged();
    return true;
}

void ClientUpdateManager::installDownloadedUpdate() {
    m_installing = true;
    emit downloadStateChanged();
    setStatusMessage(tr("正在安装更新并重启客户端…"));
    const bool started =
#if defined(Q_OS_WIN)
        startWindowsInstaller();
#elif defined(Q_OS_LINUX)
        startUbuntuInstaller();
#else
        false;
#endif
    if (!started) failDownload(tr("无法启动系统安装程序，请检查更新包和系统权限。"));
}

bool ClientUpdateManager::startWindowsInstaller() {
#if defined(Q_OS_WIN)
    const QStringList installer_arguments =
        QProcess::splitCommand(QString::fromLatin1(kWindowsInstallerArguments));
    QJsonArray arguments;
    for (const QString& argument : installer_arguments) arguments.append(argument);
    const QJsonObject payload{
        {QStringLiteral("processId"), static_cast<qint64>(QCoreApplication::applicationPid())},
        {QStringLiteral("installerPath"), QDir::toNativeSeparators(m_downloadPath)},
        {QStringLiteral("applicationPath"),
         QDir::toNativeSeparators(QCoreApplication::applicationFilePath())},
        {QStringLiteral("arguments"), arguments}};
    const QByteArray encoded_payload =
        QJsonDocument(payload).toJson(QJsonDocument::Compact).toBase64();
    const QString script =
        QStringLiteral(
            "$payload=[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('%1'));"
            "$u=ConvertFrom-Json $payload;"
            "if(Get-Process -Id $u.processId -ErrorAction SilentlyContinue){Wait-Process -Id "
            "$u.processId};"
            "$p=Start-Process -FilePath $u.installerPath -ArgumentList @($u.arguments) -Wait "
            "-PassThru;"
            "if($p.ExitCode -eq 0){Start-Process -FilePath $u.applicationPath}")
            .arg(QString::fromLatin1(encoded_payload));
    const QByteArray encoded_script(reinterpret_cast<const char*>(script.utf16()),
                                    script.size() * 2);
    const QStringList helper_arguments{QStringLiteral("-NoLogo"),
                                       QStringLiteral("-NoProfile"),
                                       QStringLiteral("-NonInteractive"),
                                       QStringLiteral("-WindowStyle"),
                                       QStringLiteral("Hidden"),
                                       QStringLiteral("-EncodedCommand"),
                                       QString::fromLatin1(encoded_script.toBase64())};
    if (!QProcess::startDetached(QStringLiteral("powershell.exe"), helper_arguments)) return false;
    emit requestApplicationClose();
    return true;
#else
    return false;
#endif
}

bool ClientUpdateManager::startUbuntuInstaller() {
#if defined(Q_OS_LINUX)
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    const QString apt_get = QStandardPaths::findExecutable(QStringLiteral("apt-get"));
    if (pkexec.isEmpty() || apt_get.isEmpty()) return false;
    m_installerProcess = new QProcess(this);
    connect(m_installerProcess, &QProcess::finished, this,
            [this](int exit_code, QProcess::ExitStatus exit_status) {
                m_installing = false;
                emit downloadStateChanged();
                if (exit_status != QProcess::NormalExit || exit_code != 0) {
                    return failDownload(tr("系统未完成更新安装，客户端保持原版本。"));
                }
                m_restartAfterQuit = true;
                emit updateSucceeded(m_releaseTag);
                setStatusMessage(tr("更新已安装，正在重启客户端…"));
                emit requestApplicationClose();
            });
    connect(m_installerProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_installing) failDownload(tr("无法启动系统更新程序。"));
    });
    m_installerProcess->start(pkexec, {apt_get, QStringLiteral("install"), QStringLiteral("--yes"),
                                       QStringLiteral("--"), m_downloadPath});
    return true;
#else
    return false;
#endif
}

void ClientUpdateManager::restartAfterQuit() {
#if defined(Q_OS_LINUX)
    if (!m_restartAfterQuit) return;
    QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
#endif
}

void ClientUpdateManager::cancelDownload() {
    if (!m_downloading || !m_downloadReply) return;
    m_cancelRequested = true;
    m_downloadReply->abort();
}

void ClientUpdateManager::failDownload(const QString& message, bool notify) {
    if (m_downloadFile) m_downloadFile->cancelWriting();
    m_downloadFile.reset();
    m_downloading = false;
    m_installing = false;
    emit downloadStateChanged();
    setStatusMessage(message);
    if (notify) emit updateFailed(message);
}

void ClientUpdateManager::setStatusMessage(const QString& message) {
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusMessageChanged();
}

void ClientUpdateManager::setChecking(bool checking) {
    if (m_checking == checking) return;
    m_checking = checking;
    emit checkingChanged();
}

QString ClientUpdateManager::currentPlatform() {
#if defined(Q_OS_WIN)
    const QString architecture = QSysInfo::currentCpuArchitecture().toLower();
    if (architecture == QStringLiteral("x86_64") || architecture == QStringLiteral("amd64"))
        return QStringLiteral("windows-x64");
#elif defined(Q_OS_LINUX)
    const QString architecture = QSysInfo::currentCpuArchitecture().toLower();
    if (QSysInfo::productType() == QStringLiteral("ubuntu") &&
        (architecture == QStringLiteral("x86_64") || architecture == QStringLiteral("amd64")))
        return QStringLiteral("ubuntu-amd64");
#else
    return {};
#endif
    return {};
}

QString ClientUpdateManager::serverCompatibilityLine(const QString& version) {
    static const QRegularExpression pattern(
        QStringLiteral("^(\\d+)\\.(\\d+)(?:\\.\\d+(?:\\.[A-Za-z][A-Za-z0-9.]*)?)?$"));
    const auto match = pattern.match(version);
    return match.hasMatch() ? match.captured(1) + QLatin1Char('.') + match.captured(2) : QString();
}

bool ClientUpdateManager::isNewerClientVersion(const QString& candidate, const QString& current) {
    static const QRegularExpression pattern(
        QStringLiteral("^(\\d+)\\.(\\d+)\\.(\\d+)(?:-([A-Za-z0-9.-]+))?$"));
    const auto candidate_match = pattern.match(candidate);
    const auto current_match = pattern.match(current);
    if (!candidate_match.hasMatch() || !current_match.hasMatch()) return true;
    for (int index = 1; index <= 3; ++index) {
        const int candidate_part = candidate_match.captured(index).toInt();
        const int current_part = current_match.captured(index).toInt();
        if (candidate_part != current_part) return candidate_part > current_part;
    }
    const QString candidate_prerelease = candidate_match.captured(4);
    const QString current_prerelease = current_match.captured(4);
    if (candidate_prerelease.isEmpty()) return !current_prerelease.isEmpty();
    if (current_prerelease.isEmpty()) return false;
    return candidate_prerelease.compare(current_prerelease, Qt::CaseInsensitive) > 0;
}
