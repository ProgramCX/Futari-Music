#ifndef FUTARI_LYRICS_COMPLETION_CONTROLLER_H
#define FUTARI_LYRICS_COMPLETION_CONTROLLER_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include "ApiClient.h"
#include "utils/RequestScope.h"

class QDir;

// 管理端逐行/逐字歌词补全的预览、匹配和提交状态唯一来源。
class LyricsCompletionController final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(LyricsCompletionController)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QStringList fileNames READ fileNames NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY changed)
    Q_PROPERTY(int candidateCount READ candidateCount NOTIFY changed)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY changed)
    Q_PROPERTY(int completed READ completed NOTIFY changed)

public:
    explicit LyricsCompletionController(ApiClient* api, QObject* parent = nullptr);

    QVariantList rows() const { return m_rows; }
    QStringList fileNames() const { return m_fileNames; }
    bool busy() const { return m_busy; }
    QString summary() const { return m_summary; }
    int selectedCount() const;
    int candidateCount() const { return m_rows.size(); }
    int pendingCount() const { return m_pending.size(); }
    int completed() const { return m_completed; }

    Q_INVOKABLE void reset(bool enabled);
    Q_INVOKABLE void scanServerSongs(const QString& type);
    Q_INVOKABLE void scanDirectory(const QUrl& directory);
    Q_INVOKABLE void selectFile(int row, int fileIndex);
    Q_INVOKABLE void submit();

signals:
    void changed();
    void errorOccurred(const QString& message);
    void completionFinished();

private:
    enum class CompletionResult { Applied, Skipped, Failed };

    void indexLyricsFiles(const QDir& directory);
    void clearFileIndex();
    void applyFileMatches();
    void updateScanSummary();
    void loadCandidates(int page, const RequestScope::Token& token);
    void receiveCandidates(int page, const QJsonObject& result,
                           const RequestScope::Token& token);
    void appendCandidate(const QJsonObject& song);
    QList<int> matchFiles(const QString& title, const QString& artist) const;
    void uploadNext(const RequestScope::Token& token);
    void finishRow(CompletionResult result, const QString& status,
                   const RequestScope::Token& token);
    static QString matchKey(const QString& text);
    static bool readLyricsFile(const QString& path, const QString& type, QString* lyrics);
    static QString previewText(const QString& lyrics);

    ApiClient* m_api;
    RequestScope m_requests;
    QVariantList m_rows;
    QStringList m_fileNames;
    QStringList m_filePaths;
    QStringList m_filePreviews;
    QHash<QString, QList<int>> m_filesByName;
    // 上传期间固定待处理行快照；失败行保持可重试，成功/跳过行不重复提交。
    QList<int> m_pending;
    QString m_type;
    QString m_summary;
    // busy 同时覆盖服务器分页和串行上传，避免用户编辑已确认的匹配。
    bool m_busy = false;
    bool m_submitting = false;
    bool m_enabled = false;
    int m_completed = 0;
    QString m_directory;
};

#endif  // FUTARI_LYRICS_COMPLETION_CONTROLLER_H
