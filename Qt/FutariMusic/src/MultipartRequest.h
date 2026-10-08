#pragma once

#include <QHttpMultiPart>
#include <QNetworkRequest>

// Qt 的自动边界可能包含 /、+ 等字符，参数使用 quoted-string 而非裸 token。
inline void setMultipartContentType(QNetworkRequest &request, const QHttpMultiPart &parts) {
    QByteArray boundary = parts.boundary();
    boundary.replace("\\", "\\\\");
    boundary.replace("\"", "\\\"");
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QByteArray("multipart/form-data; boundary=\"") + boundary + '"');
}
