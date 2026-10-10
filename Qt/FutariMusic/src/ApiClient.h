#ifndef FUTARI_API_CLIENT_H
#define FUTARI_API_CLIENT_H

#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>
#include <functional>

class QHttpMultiPart;

// HTTP 通信边界：统一认证、响应信封和错误处理。
class ApiClient final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(ApiClient)
public:
    using Callback = std::function<void(const QJsonValue &)>;
    using ErrorCallback = std::function<void()>;
    struct PageQuery {
        int page = 1;
        int pageSize = 40;
        QString keyword;
    };
    using PageCallback = std::function<void(const QJsonObject&)>;
    void pagedGet(const QString& path, const PageQuery& query, PageCallback onSuccess,
                  ErrorCallback onError = {});
    explicit ApiClient(QObject *parent = nullptr);
    void setBaseUrl(const QUrl &url);
    void setToken(const QString &token);
    void abortAll(QNetworkReply *except = nullptr);
    QUrl baseUrl() const { return m_baseUrl; }
    QString token() const { return m_token; }
    QNetworkReply *request(const QByteArray &method, const QString &path,
                           const QJsonObject &body, Callback onSuccess, ErrorCallback onError = {});
    void upload(const QString &path, QHttpMultiPart *parts, Callback onSuccess, ErrorCallback onError = {});
    void putUpload(const QString &path, QHttpMultiPart *parts, Callback onSuccess);
    void download(const QString &path, Callback onSuccess);
signals:
    void errorOccurred(const QString &message);
    void unauthorized();
private:
    QNetworkRequest makeRequest(const QString &path) const;
    void finishJson(QNetworkReply *reply, Callback onSuccess, ErrorCallback onError = {});
    QNetworkAccessManager m_network;
    QUrl m_baseUrl;
    QString m_token;
};

#endif  // FUTARI_API_CLIENT_H
