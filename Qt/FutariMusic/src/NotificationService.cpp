#include "NotificationService.h"

#include <QApplication>
#include <QStyle>

NotificationService::NotificationService(QObject *parent) : QObject(parent) {
    m_tray.setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    m_tray.setToolTip(QStringLiteral("Futari Music"));
    connect(&m_tray, &QSystemTrayIcon::messageClicked, this, &NotificationService::activated);
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) emit activated();
    });
    if (QSystemTrayIcon::isSystemTrayAvailable()) m_tray.show();
}

void NotificationService::showInvitation(const QString &title, const QString &message) {
    if (m_tray.isVisible()) m_tray.showMessage(title, message, QSystemTrayIcon::Information, 8000);
}
