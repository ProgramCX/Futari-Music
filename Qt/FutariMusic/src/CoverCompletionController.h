#ifndef FUTARI_COVER_COMPLETION_CONTROLLER_H
#define FUTARI_COVER_COMPLETION_CONTROLLER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantList>

#include "ApiClient.h"
#include "utils/RequestScope.h"

// 补全预览及提交的唯一状态源；切换账号/服务器立即使旧扫描和回调失效。
class CoverCompletionController final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(CoverCompletionController)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QStringList imageNames READ imageNames NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY changed)
    Q_PROPERTY(int completed READ completed NOTIFY changed)
    Q_PROPERTY(int total READ total NOTIFY changed)
public:
    explicit CoverCompletionController(ApiClient* api, QObject* parent = nullptr);
    QVariantList rows() const { return m_rows; }
    QStringList imageNames() const { return m_imageNames; }
    bool busy() const { return m_busy; }
    QString summary() const { return m_summary; }
    int selectedCount() const;
    int completed() const { return m_completed; }
    int total() const { return m_pending.size(); }
    void reset(bool enabled);
    Q_INVOKABLE void scan(const QUrl& directory);
    Q_INVOKABLE void selectImage(int row, int imageIndex);
    Q_INVOKABLE void submit();
signals:
    void changed();
    void errorOccurred(const QString& message);
    void completionFinished();

private:
    enum class CompletionResult { Applied, Skipped, Failed };
    void loadCandidates(const QString& kind, int page, const RequestScope::Token& token);
    void receiveCandidates(const QString& kind, int page, const QJsonObject& result,
                           const RequestScope::Token& token);
    void appendCandidates(const QString& kind, const QJsonArray& candidates);
    void appendSong(const QJsonObject& song);
    void appendAlbum(const QJsonObject& album);
    QList<int> matchImages(const QString& title, const QString& artist) const;
    void uploadNext(const RequestScope::Token& token);
    void finishRow(CompletionResult result, const QString& status,
                   const RequestScope::Token& token);
    static QString matchKey(const QString& text);
    ApiClient* m_api;
    RequestScope m_requests;
    QVariantList m_rows;
    QStringList m_imageNames;
    QStringList m_imagePaths;
    QHash<QString, QList<int>> m_imagesByName;
    QHash<qint64, QList<int>> m_albumSuggestions;
    QList<int> m_pending;
    // busy 覆盖扫描与提交，防止修改正在发送的确认快照。
    bool m_busy = false;
    bool m_enabled = false;
    int m_completed = 0;
    QString m_summary;
};

#endif
