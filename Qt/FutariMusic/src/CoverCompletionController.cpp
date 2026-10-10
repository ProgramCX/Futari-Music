#include "CoverCompletionController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTimer>

namespace {
constexpr qint64 kMaxCoverBytes = 5LL * 1024 * 1024;
constexpr int kPageSize = 100;
}  // namespace

CoverCompletionController::CoverCompletionController(ApiClient* api, QObject* parent)
    : QObject(parent), m_api(api) {}

void CoverCompletionController::reset(bool enabled) {
    m_requests.renew();
    m_enabled = enabled;
    m_busy = false;
    m_rows.clear();
    m_imageNames = {QStringLiteral("不补全此项")};
    m_imagePaths = {QString()};
    m_imagesByName.clear();
    m_albumSuggestions.clear();
    m_pending.clear();
    m_completed = 0;
    m_summary.clear();
    emit changed();
}

QString CoverCompletionController::matchKey(const QString& text) {
    // 比较完整文件名而非按第一个 '-' 拆开，歌曲名本身可以包含连字符。
    QString key = text.normalized(QString::NormalizationForm_KC).trimmed().toCaseFolded();
    key.replace(QRegularExpression(QStringLiteral("\\s*-\\s*")), "-");
    return key.simplified();
}

void CoverCompletionController::scan(const QUrl& directory) {
    if (!m_enabled || m_busy) return;
    const QDir folder(directory.toLocalFile());
    if (!directory.isLocalFile() || !folder.exists() ||
        !QFileInfo(folder.absolutePath()).isReadable()) {
        emit errorOccurred(QStringLiteral("请选择可读取的本地图片目录"));
        return;
    }
    reset(true);
    const auto files = folder.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo& file : files) {
        const QString suffix = file.suffix().toLower();
        if (!QStringList{"jpg", "jpeg", "png", "webp"}.contains(suffix) || file.size() <= 0 ||
            file.size() > kMaxCoverBytes)
            continue;
        const int index = m_imagePaths.size();
        m_imagePaths.append(file.absoluteFilePath());
        m_imageNames.append(file.fileName());
        m_imagesByName[matchKey(file.completeBaseName())].append(index);
    }
    if (m_imagePaths.size() == 1) {
        m_summary = QStringLiteral("目录内没有可用图片（支持 JPG、PNG、WebP，每张不超过 5 MiB）");
        emit changed();
        return;
    }
    m_busy = true;
    m_summary = QStringLiteral("正在读取服务器缺失封面…");
    emit changed();
    loadCandidates("songs", 1, m_requests.token());
}

QList<int> CoverCompletionController::matchImages(const QString& title,
                                                  const QString& artist) const {
    if (title.trimmed().isEmpty() || artist.trimmed().isEmpty()) return {};
    auto matches = m_imagesByName.value(matchKey(title + "-" + artist));
    if (matches.isEmpty()) matches = m_imagesByName.value(matchKey(artist + "-" + title));
    return matches;
}

void CoverCompletionController::loadCandidates(const QString& kind, int page,
                                               const RequestScope::Token& token) {
    m_api->pagedGet(
        "api/cover-completion/" + kind, {page, kPageSize, {}},
        [this, kind, page, token](const QJsonObject& value) {
            if (token.expired()) return;
            receiveCandidates(kind, page, value, token);
        },
        [this, token] {
            if (token.expired()) return;
            m_busy = false;
            m_rows.clear();
            m_summary = QStringLiteral("读取失败，请重新选择目录重试");
            emit changed();
        });
}

void CoverCompletionController::receiveCandidates(const QString& kind, int page,
                                                  const QJsonObject& result,
                                                  const RequestScope::Token& token) {
    const auto candidates = result.value("list").toArray();
    appendCandidates(kind, candidates);
    if (!candidates.isEmpty() && page * kPageSize < result.value("total").toInteger()) {
        loadCandidates(kind, page + 1, token);
    } else if (kind == "songs") {
        loadCandidates("albums", 1, token);
    } else {
        m_busy = false;
        m_summary = QStringLiteral("找到 %1 个缺失封面。请检查歌曲匹配，并单独选择专辑封面。")
                        .arg(m_rows.size());
        emit changed();
    }
}

void CoverCompletionController::appendCandidates(const QString& kind,
                                                 const QJsonArray& candidates) {
    for (const auto& value : candidates) {
        if (kind == "songs")
            appendSong(value.toObject());
        else
            appendAlbum(value.toObject());
    }
}

void CoverCompletionController::appendSong(const QJsonObject& song) {
    const auto matches =
        matchImages(song.value("title").toString(), song.value("artist").toString());
    const qint64 albumId = song.value("albumId").toInteger();
    if (song.value("albumMissing").toBool()) {
        auto& suggestions = m_albumSuggestions[albumId];
        for (int match : matches)
            if (!suggestions.contains(match)) suggestions.append(match);
    }
    if (!song.value("songMissing").toBool()) return;
    const int imageIndex = matches.size() == 1 ? matches.first() : 0;
    m_rows.append(QVariantMap{
        {"id", song.value("id").toInteger()},
        {"kind", "songs"},
        {"title", song.value("title").toString()},
        {"artist", song.value("artist").toString()},
        {"imageIndex", imageIndex},
        {"imageUrl",
         imageIndex ? QUrl::fromLocalFile(m_imagePaths[imageIndex]).toString() : QString()},
        {"status", matches.size() > 1 ? QStringLiteral("多个匹配，请选择")
                   : imageIndex       ? QStringLiteral("已匹配，待确认")
                                      : QStringLiteral("未匹配，可手动选择")}});
}

void CoverCompletionController::appendAlbum(const QJsonObject& album) {
    const auto suggestions = m_albumSuggestions.value(album.value("id").toInteger());
    QStringList names;
    for (int index : suggestions) names.append(m_imageNames[index]);
    m_rows.append(QVariantMap{{"id", album.value("id").toInteger()},
                              {"kind", "albums"},
                              {"title", album.value("name").toString()},
                              {"artist", album.value("artist").toString()},
                              {"imageIndex", 0},
                              {"imageUrl", ""},
                              {"status", names.isEmpty() ? QStringLiteral("专辑封面需单独选择")
                                                         : QStringLiteral("歌曲匹配候选：") +
                                                               names.join(QStringLiteral("、"))}});
}

int CoverCompletionController::selectedCount() const {
    int count = 0;
    for (const auto& value : m_rows) {
        const auto row = value.toMap();
        if (row.value("imageIndex").toInt() > 0 && !row.value("done").toBool()) ++count;
    }
    return count;
}

void CoverCompletionController::selectImage(int rowIndex, int imageIndex) {
    if (m_busy || !m_enabled || rowIndex < 0 || rowIndex >= m_rows.size() || imageIndex < 0 ||
        imageIndex >= m_imagePaths.size())
        return;
    auto row = m_rows[rowIndex].toMap();
    if (row.value("done").toBool()) return;
    row["imageIndex"] = imageIndex;
    row["imageUrl"] =
        imageIndex ? QUrl::fromLocalFile(m_imagePaths[imageIndex]).toString() : QString();
    row["status"] = imageIndex ? QStringLiteral("已选择，待确认") : QStringLiteral("不补全此项");
    m_rows[rowIndex] = row;
    emit changed();
}

void CoverCompletionController::submit() {
    if (m_busy || !m_enabled) return;
    m_pending.clear();
    m_completed = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const auto row = m_rows[i].toMap();
        if (row.value("imageIndex").toInt() > 0 && !row.value("done").toBool()) m_pending.append(i);
    }
    if (m_pending.isEmpty()) return;
    m_busy = true;
    m_summary = QStringLiteral("正在补全封面…");
    emit changed();
    uploadNext(m_requests.token());
}

void CoverCompletionController::uploadNext(const RequestScope::Token& token) {
    if (token.expired()) return;
    if (m_completed == m_pending.size()) {
        m_busy = false;
        m_summary = QStringLiteral("补全结束，请查看逐项结果；失败项可以重试。");
        emit changed();
        emit completionFinished();
        return;
    }
    const auto row = m_rows[m_pending[m_completed]].toMap();
    const QString path = m_imagePaths[row.value("imageIndex").toInt()];
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > kMaxCoverBytes) {
        finishRow(CompletionResult::Failed, QStringLiteral("失败：图片不可读或超过 5 MiB"), token);
        return;
    }
    auto parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    // 上传名只保留格式，避免本地路径/引号进入 multipart 头。
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"file\"; filename=\"cover.%1\"")
                       .arg(QFileInfo(path).suffix().toLower()));
    part.setBody(file.read(kMaxCoverBytes + 1));
    parts->append(part);
    const QString endpoint = QStringLiteral("api/cover-completion/%1/%2")
                                 .arg(row.value("kind").toString())
                                 .arg(row.value("id").toLongLong());
    m_api->upload(
        endpoint, parts,
        [this, token](const QJsonValue& value) {
            if (token.expired()) return;
            const bool applied = value.toObject().value("applied").toBool();
            finishRow(applied ? CompletionResult::Applied : CompletionResult::Skipped,
                      applied ? QStringLiteral("已补全") : QStringLiteral("已有封面，未覆盖"),
                      token);
        },
        [this, token] {
            if (!token.expired())
                finishRow(CompletionResult::Failed, QStringLiteral("失败：请检查错误提示后重试"),
                          token);
        });
}

void CoverCompletionController::finishRow(CompletionResult result, const QString& status,
                                          const RequestScope::Token& token) {
    if (token.expired()) return;
    const int index = m_pending[m_completed];
    auto row = m_rows[index].toMap();
    row["status"] = status;
    row["done"] = result != CompletionResult::Failed;
    // 保留确认过的图片供查看结果；done 项不再计入待提交数量。
    m_rows[index] = row;
    ++m_completed;
    emit changed();
    // 顺序请求既限制内存，也让失败文件不会阻塞其余确认项。
    QTimer::singleShot(0, this, [this, token] { uploadNext(token); });
}
