#include "LyricsCompletionController.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTimer>
#include <algorithm>

#include "utils/LyricsParser.h"

namespace {
constexpr qint64 kMaxLyricsBytes = 256LL * 1024;
constexpr int kPageSize = 100;
}  // namespace

LyricsCompletionController::LyricsCompletionController(ApiClient* api, QObject* parent)
    : QObject(parent), m_api(api) {}

void LyricsCompletionController::reset(bool enabled) {
    m_requests.renew();
    m_enabled = enabled;
    m_busy = false;
    m_submitting = false;
    m_rows.clear();
    clearFileIndex();
    m_pending.clear();
    m_type.clear();
    m_directory.clear();
    m_summary.clear();
    m_completed = 0;
    emit changed();
}

QString LyricsCompletionController::matchKey(const QString& text) {
    QString key = text.normalized(QString::NormalizationForm_KC).trimmed().toCaseFolded();
    key.replace(QRegularExpression(QStringLiteral("\\s*-\\s*")), "-");
    return key.simplified();
}

bool LyricsCompletionController::readLyricsFile(const QString& path, const QString& type,
                                                 QString* lyrics) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > kMaxLyricsBytes)
        return false;
    QByteArray bytes = file.read(kMaxLyricsBytes + 1);
    if (bytes.isEmpty() || bytes.size() > kMaxLyricsBytes) return false;
    // Qt 解码 UTF-8 时会跳过开头的 BOM；先移除字节标记，再做往返校验，避免误拒绝常见的带 BOM 歌词文件。
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes.remove(0, 3);
    QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes) return false;
    const bool isKaraoke = LyricsParser::hasKaraokeTimestamps(decoded);
    if (decoded.trimmed().isEmpty() || isKaraoke != (type == QStringLiteral("karaoke")))
        return false;
    *lyrics = decoded;
    return true;
}

QString LyricsCompletionController::previewText(const QString& lyrics) {
    static const QRegularExpression lineTags(R"(\[\d{1,3}:\d{1,2}(?:\.\d+)?\])");
    static const QRegularExpression inlineTags(R"(<\d{1,3}:\d{1,2}(?:\.\d+)?>)");
    static const QRegularExpression metadata(
        R"(^\s*\[(?:ti|ar|al|by|re|ve|offset|kana|la|length):.*\]\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    QStringList lines;
    for (QString line : lyrics.split(QRegularExpression(QStringLiteral("\\r?\\n")))) {
        if (metadata.match(line).hasMatch()) continue;
        line.remove(lineTags);
        line.remove(inlineTags);
        line = line.simplified();
        if (!line.isEmpty()) lines.append(line);
        if (lines.size() == 2) break;
    }
    return lines.join(QStringLiteral(" / ")).left(160);
}

void LyricsCompletionController::scanServerSongs(const QString& type) {
    if (!m_enabled || m_busy || m_submitting) return;
    if (type != QStringLiteral("line") && type != QStringLiteral("karaoke")) {
        emit errorOccurred(QStringLiteral("请选择逐行或逐字歌词类型"));
        return;
    }
    reset(true);
    m_type = type;
    m_busy = true;
    m_summary = QStringLiteral("正在读取服务器缺少的歌词…");
    emit changed();
    loadCandidates(1, m_requests.token());
}

void LyricsCompletionController::scanDirectory(const QUrl& directory) {
    if (!m_enabled || m_submitting || m_type.isEmpty()) return;
    const QDir folder(directory.toLocalFile());
    if (!directory.isLocalFile() || !folder.exists() ||
        !QFileInfo(folder.absolutePath()).isReadable()) {
        emit errorOccurred(QStringLiteral("请选择可读取的本地歌词目录"));
        return;
    }
    clearFileIndex();
    m_pending.clear();
    m_completed = 0;
    m_directory = folder.absolutePath();
    indexLyricsFiles(folder);
    applyFileMatches();
    updateScanSummary();
    emit changed();
}

void LyricsCompletionController::clearFileIndex() {
    m_fileNames = {QStringLiteral("不补全此项")};
    m_filePaths = {QString()};
    m_filePreviews = {QString()};
    m_filesByName.clear();
}

void LyricsCompletionController::indexLyricsFiles(const QDir& directory) {
    QDirIterator iterator(directory.absolutePath(), QDir::Files | QDir::Readable | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
    QStringList paths;
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == QStringLiteral("lrc") || suffix == QStringLiteral("txt")) paths.append(path);
    }
    std::sort(paths.begin(), paths.end(), [](const QString& left, const QString& right) {
        return QString::compare(left, right, Qt::CaseInsensitive) < 0;
    });
    for (const QString& path : paths) {
        QString lyrics;
        if (!readLyricsFile(path, m_type, &lyrics)) continue;
        const int index = m_filePaths.size();
        m_filePaths.append(path);
        m_fileNames.append(directory.relativeFilePath(path));
        m_filePreviews.append(previewText(lyrics));
        m_filesByName[matchKey(QFileInfo(path).completeBaseName())].append(index);
    }
}

void LyricsCompletionController::applyFileMatches() {
    for (int index = 0; index < m_rows.size(); ++index) {
        QVariantMap row = m_rows.at(index).toMap();
        const QList<int> matches = matchFiles(row.value("title").toString(),
                                              row.value("artist").toString());
        const int fileIndex = matches.size() == 1 ? matches.first() : 0;
        row["fileIndex"] = fileIndex;
        row["preview"] = fileIndex > 0 ? m_filePreviews.at(fileIndex) : QString();
        row["done"] = false;
        row["status"] = matches.size() > 1 ? QStringLiteral("多个文件匹配，请手动选择")
                   : fileIndex > 0    ? QStringLiteral("已匹配，待确认")
                   : m_filePaths.size() == 1
                       ? QStringLiteral("目录内没有符合类型的歌词文件")
                       : QStringLiteral("未匹配，可手动选择");
        m_rows[index] = row;
    }
}

void LyricsCompletionController::updateScanSummary() {
    const QString typeName = m_type == QStringLiteral("karaoke") ? QStringLiteral("逐字")
                                                                 : QStringLiteral("逐行");
    if (m_directory.isEmpty()) {
        m_summary = QStringLiteral("找到 %1 首缺少%2歌词的歌曲，请选择歌词目录进行匹配。")
                        .arg(m_rows.size())
                        .arg(typeName);
    } else if (m_filePaths.size() == 1) {
        m_summary = QStringLiteral("找到 %1 首缺少%2歌词的歌曲，但目录内没有符合类型的歌词文件。")
                        .arg(m_rows.size())
                        .arg(typeName);
    } else {
        m_summary = QStringLiteral("找到 %1 首缺少%2歌词的歌曲，请检查匹配结果后确认。")
                        .arg(m_rows.size())
                        .arg(typeName);
    }
}

QList<int> LyricsCompletionController::matchFiles(const QString& title,
                                                  const QString& artist) const {
    if (title.trimmed().isEmpty() || artist.trimmed().isEmpty()) return {};
    auto matches = m_filesByName.value(matchKey(title + "-" + artist));
    if (matches.isEmpty()) matches = m_filesByName.value(matchKey(artist + "-" + title));
    return matches;
}

void LyricsCompletionController::loadCandidates(int page, const RequestScope::Token& token) {
    m_api->pagedGet(
        "api/lyrics-completion/" + m_type, {page, kPageSize, {}},
        [this, page, token](const QJsonObject& result) {
            if (!token.expired()) receiveCandidates(page, result, token);
        },
        [this, token] {
            if (token.expired()) return;
            m_busy = false;
            m_rows.clear();
            m_summary = QStringLiteral("读取服务器歌曲失败，请检查网络后重新扫描");
            emit changed();
        });
}

void LyricsCompletionController::receiveCandidates(int page, const QJsonObject& result,
                                                   const RequestScope::Token& token) {
    const QJsonArray candidates = result.value("list").toArray();
    for (const QJsonValue& value : candidates) appendCandidate(value.toObject());
    if (!candidates.isEmpty() && page * kPageSize < result.value("total").toInteger()) {
        loadCandidates(page + 1, token);
        return;
    }
    m_busy = false;
    updateScanSummary();
    emit changed();
}

void LyricsCompletionController::appendCandidate(const QJsonObject& song) {
    const QString title = song.value("title").toString();
    const QString artist = song.value("artist").toString();
    const QList<int> matches = matchFiles(title, artist);
    const int fileIndex = matches.size() == 1 ? matches.first() : 0;
    m_rows.append(QVariantMap{
        {"id", song.value("id").toInteger()},
        {"title", title},
        {"artist", artist},
        {"fileIndex", fileIndex},
        {"preview", fileIndex > 0 ? m_filePreviews.at(fileIndex) : QString()},
        {"status", matches.size() > 1 ? QStringLiteral("多个文件匹配，请手动选择")
                   : fileIndex > 0    ? QStringLiteral("已匹配，待确认")
                   : m_directory.isEmpty() ? QStringLiteral("选择歌词目录后自动匹配")
                   : m_filePaths.size() == 1
                       ? QStringLiteral("目录内没有符合类型的歌词文件")
                       : QStringLiteral("未匹配，可手动选择")},
        {"done", false}});
}

int LyricsCompletionController::selectedCount() const {
    int count = 0;
    for (const QVariant& value : m_rows) {
        const QVariantMap row = value.toMap();
        if (row.value("fileIndex").toInt() > 0 && !row.value("done").toBool()) ++count;
    }
    return count;
}

void LyricsCompletionController::selectFile(int rowIndex, int fileIndex) {
    if (m_busy || !m_enabled || rowIndex < 0 || rowIndex >= m_rows.size() || fileIndex < 0 ||
        fileIndex >= m_filePaths.size())
        return;
    QVariantMap row = m_rows.at(rowIndex).toMap();
    if (row.value("done").toBool()) return;
    row["fileIndex"] = fileIndex;
    row["preview"] = fileIndex > 0 ? m_filePreviews.at(fileIndex) : QString();
    row["status"] = fileIndex > 0 ? QStringLiteral("已选择，待确认") : QStringLiteral("不补全此项");
    m_rows[rowIndex] = row;
    emit changed();
}

void LyricsCompletionController::submit() {
    if (m_busy || !m_enabled) return;
    m_pending.clear();
    m_completed = 0;
    for (int index = 0; index < m_rows.size(); ++index) {
        const QVariantMap row = m_rows.at(index).toMap();
        if (row.value("fileIndex").toInt() > 0 && !row.value("done").toBool())
            m_pending.append(index);
    }
    if (m_pending.isEmpty()) return;
    m_busy = true;
    m_submitting = true;
    m_summary = QStringLiteral("正在补全歌词…");
    emit changed();
    uploadNext(m_requests.token());
}

void LyricsCompletionController::uploadNext(const RequestScope::Token& token) {
    if (token.expired()) return;
    if (m_completed == m_pending.size()) {
        m_busy = false;
        m_submitting = false;
        m_summary = QStringLiteral("补全结束，请查看逐项结果；失败项可以重试。需重新扫描才能查看最新服务器缺失项。");
        emit changed();
        emit completionFinished();
        return;
    }
    const QVariantMap row = m_rows.at(m_pending.at(m_completed)).toMap();
    const int fileIndex = row.value("fileIndex").toInt();
    const QString path = m_filePaths.at(fileIndex);
    QString lyrics;
    if (!readLyricsFile(path, m_type, &lyrics)) {
        finishRow(CompletionResult::Failed, QStringLiteral("失败：歌词文件不可读、格式不符或超过 256 KiB"), token);
        return;
    }
    const QString suffix = QFileInfo(path).suffix().toLower();
    auto* parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"file\"; filename=\"lyrics.%1\"")
                       .arg(suffix));
    part.setBody(lyrics.toUtf8());
    parts->append(part);
    const QString endpoint = QStringLiteral("api/lyrics-completion/%1/%2")
                                 .arg(m_type)
                                 .arg(row.value("id").toLongLong());
    m_api->upload(
        endpoint, parts,
        [this, token](const QJsonValue& value) {
            if (token.expired()) return;
            const bool applied = value.toObject().value("applied").toBool();
            finishRow(applied ? CompletionResult::Applied : CompletionResult::Skipped,
                      applied ? QStringLiteral("已补全") : QStringLiteral("服务器已有歌词，未覆盖"),
                      token);
        },
        [this, token] {
            if (!token.expired())
                finishRow(CompletionResult::Failed, QStringLiteral("失败：请检查错误提示后重试"), token);
        });
}

void LyricsCompletionController::finishRow(CompletionResult result, const QString& status,
                                           const RequestScope::Token& token) {
    if (token.expired()) return;
    QVariantMap row = m_rows.at(m_pending.at(m_completed)).toMap();
    row["status"] = status;
    row["done"] = result != CompletionResult::Failed;
    m_rows[m_pending.at(m_completed)] = row;
    ++m_completed;
    emit changed();
    QTimer::singleShot(0, this, [this, token] { uploadNext(token); });
}
