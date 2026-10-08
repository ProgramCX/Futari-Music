#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class AudioMetadataReader final : public QObject {
    Q_OBJECT
public:
    explicit AudioMetadataReader(QObject *parent = nullptr);
    Q_INVOKABLE void inspect(const QString &audioUrl, const QString &requestId);
signals:
    void metadataReady(const QString &requestId, const QVariantMap &metadata);
private:
    void finish();
    QMediaPlayer m_player;
    QTimer m_settleTimer;
    QTimer m_timeoutTimer;
    QString m_audioUrl;
    QString m_requestId;
};
