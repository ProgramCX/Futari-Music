#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class NotificationService final : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject *parent = nullptr);
    void showInvitation(const QString &title, const QString &message);
signals:
    void activated();
private:
    QSystemTrayIcon m_tray;
};
