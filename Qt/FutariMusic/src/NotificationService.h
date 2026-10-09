#pragma once

#include <QIcon>
#include <QObject>
#include <QSystemTrayIcon>

class NotificationService final : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject* parent = nullptr);
    void showInvitation(const QString& title, const QString& message);
    void showUpdateStarted(const QString& release_tag);
    void showUpdateSucceeded(const QString& release_tag);
    void showUpdateFailed(const QString& message);
    void setUpdateProgress(int percentage, bool indeterminate);
    void showUpdateInstalling();
    void clearUpdateProgress();
signals:
    void activated();

private:
    QIcon m_baseIcon;
    QSystemTrayIcon m_tray;
    int m_lastProgress = -2;
    bool m_lastProgressIndeterminate = false;
};
