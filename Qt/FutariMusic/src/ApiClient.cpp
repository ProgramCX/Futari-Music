#include "ApiClient.h"

#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrlQuery>

#include "MultipartRequest.h"

namespace {
constexpr char kSessionCancelledProperty[] = "futariSessionCancelled";
}

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

void ApiClient::setBaseUrl(const QUrl &url) {
    m_baseUrl = url;
    QString path = m_baseUrl.path();
    if (!path.endsWith('/')) path += '/';
    m_baseUrl.setPath(path);
}

void ApiClient::setToken(const QString &token) { m_token = token; }
void ApiClient::abortAll(QNetworkReply *except) {
    for (QNetworkReply *reply : m_network.findChildren<QNetworkReply *>()) {
        if (reply != except && reply->isRunning()) {
            // 只静默主动取消；超时也可能返回 OperationCanceledError，仍需报告给用户。
            reply->setProperty(kSessionCancelledProperty, true);
            reply->abort();
        }
    }
}

QNetworkRequest ApiClient::makeRequest(const QString &path) const {
    const QUrl url = m_baseUrl.resolved(QUrl(path.startsWith('/') ? path.mid(1) : path));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    if (!m_token.isEmpty()) request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setTransferTimeout(30000);
    return request;
}

QNetworkReply *ApiClient::request(const QByteArray &method, const QString &path,
                                  const QJsonObject &body, Callback onSuccess, ErrorCallback onError) {
    QNetworkRequest request = makeRequest(path);
    QNetworkReply *reply = nullptr;
    if (method == "GET") reply = m_network.get(request);
    else {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
        reply = m_network.sendCustomRequest(request, method, payload);
    }
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)] {
                finishJson(reply, onSuccess, onError);
            });
    return reply;
}

void ApiClient::upload(const QString &path, QHttpMultiPart *parts, Callback onSuccess, ErrorCallback onError) {
    QNetworkRequest request = makeRequest(path);
    setMultipartContentType(request, *parts);
    QNetworkReply *reply = m_network.post(request, parts);
    parts->setParent(reply);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)] {
                finishJson(reply, onSuccess, onError);
            });
}

void ApiClient::putUpload(const QString &path, QHttpMultiPart *parts, Callback onSuccess) {
    QNetworkRequest request = makeRequest(path);
    setMultipartContentType(request, *parts);
    QNetworkReply *reply = m_network.sendCustomRequest(request, "PUT", parts);
    parts->setParent(reply);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, onSuccess = std::move(onSuccess)] { finishJson(reply, onSuccess); });
}

void ApiClient::download(const QString &path, Callback onSuccess) {
    QNetworkReply *reply = m_network.get(makeRequest(path));
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess)] {
        if (reply->property(kSessionCancelledProperty).toBool()) {
            reply->deleteLater();
            return;
        }
        const QByteArray bytes = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401)
            emit unauthorized();
        else if (reply->error() != QNetworkReply::NoError || status >= 400)
            emit errorOccurred(reply->errorString());
        else if (onSuccess)
            onSuccess(QString::fromLatin1(bytes.toBase64()));
        reply->deleteLater();
    });
}

void ApiClient::finishJson(QNetworkReply *reply, Callback onSuccess, ErrorCallback onError) {
    if (reply->property(kSessionCancelledProperty).toBool()) {
        reply->deleteLater();
        return;
    }
    const QByteArray bytes = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    const QJsonObject envelope = document.object();
    if (status == 401) { emit unauthorized(); if (onError) onError(); }
    else if (reply->error() != QNetworkReply::NoError || status >= 400 ||
             parseError.error != QJsonParseError::NoError || envelope.value("code").toInt(-1) != 0) {
        const QString message = envelope.value("message").toString();
        emit errorOccurred(message.isEmpty() ? reply->errorString() : message);
        if (onError) onError();
    } else if (onSuccess) onSuccess(envelope.value("data"));
    reply->deleteLater();
}

void ApiClient::pagedGet(const QString& path, const PageQuery& pageQuery, PageCallback onSuccess,
                         ErrorCallback onError) {
    QUrl url(path);
    QUrlQuery query(url);
    query.addQueryItem("pageNum", QString::number(pageQuery.page));
    query.addQueryItem("pageSize", QString::number(pageQuery.pageSize));
    if (!pageQuery.keyword.isEmpty())
        query.addQueryItem("keyword",
                           QString::fromLatin1(QUrl::toPercentEncoding(pageQuery.keyword)));
    url.setQuery(query);
    request(
        "GET", url.toString(QUrl::FullyEncoded), {},
        [onSuccess = std::move(onSuccess)](const QJsonValue& value) {
            if (onSuccess) onSuccess(value.toObject());
        },
        std::move(onError));
}
