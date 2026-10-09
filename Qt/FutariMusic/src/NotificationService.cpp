#include "NotificationService.h"

#include <QApplication>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QStyle>

NotificationService::NotificationService(QObject* parent) : QObject(parent) {
    m_baseIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    m_tray.setIcon(m_baseIcon);
    m_tray.setToolTip(QStringLiteral("Futari Music"));
    connect(&m_tray, &QSystemTrayIcon::messageClicked, this, &NotificationService::activated);
    connect(&m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                    emit activated();
            });
    if (QSystemTrayIcon::isSystemTrayAvailable()) m_tray.show();
}

void NotificationService::showInvitation(const QString& title, const QString& message) {
    if (m_tray.isVisible()) m_tray.showMessage(title, message, QSystemTrayIcon::Information, 8000);
}

void NotificationService::showUpdateStarted(const QString& release_tag) {
    if (m_tray.isVisible()) {
        m_tray.showMessage(QStringLiteral("Futari Music 更新"),
                           QStringLiteral("正在下载客户端版本 %1").arg(release_tag),
                           QSystemTrayIcon::Information, 8000);
    }
    m_lastProgress = -2;
    setUpdateProgress(0, true);
}

void NotificationService::showUpdateSucceeded(const QString& release_tag) {
    if (m_tray.isVisible()) {
        m_tray.showMessage(QStringLiteral("客户端更新完成"),
                           QStringLiteral("版本 %1 已安装，客户端即将重启。").arg(release_tag),
                           QSystemTrayIcon::Information, 8000);
    }
    clearUpdateProgress();
}

void NotificationService::showUpdateFailed(const QString& message) {
    if (m_tray.isVisible()) {
        m_tray.showMessage(QStringLiteral("客户端更新失败"), message, QSystemTrayIcon::Warning,
                           10000);
    }
    clearUpdateProgress();
}

void NotificationService::setUpdateProgress(int percentage, bool indeterminate) {
    if (!m_tray.isVisible() ||
        (m_lastProgress == percentage && m_lastProgressIndeterminate == indeterminate))
        return;
    m_lastProgress = percentage;
    m_lastProgressIndeterminate = indeterminate;
    const QString label = indeterminate ? QStringLiteral("正在下载客户端更新")
                                        : QStringLiteral("正在下载客户端更新 %1%").arg(percentage);
    m_tray.setToolTip(QStringLiteral("Futari Music · %1").arg(label));

    QPixmap icon = m_baseIcon.pixmap(32, 32);
    icon = icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF ring(2.5, 2.5, 27, 27);
    painter.setPen(QPen(QColor(125, 132, 145, 100), 3));
    painter.drawEllipse(ring);
    painter.setPen(QPen(QColor(19, 186, 120), 3, Qt::SolidLine, Qt::RoundCap));
    const int span = indeterminate ? 100 * 16 : qBound(1, percentage, 100) * 360 * 16 / 100;
    painter.drawArc(ring, 90 * 16, -span);
    m_tray.setIcon(QIcon(icon));
}

void NotificationService::showUpdateInstalling() {
    if (!m_tray.isVisible()) return;
    m_tray.setToolTip(QStringLiteral("Futari Music · 正在安装客户端更新"));
    setUpdateProgress(100, false);
    m_tray.setToolTip(QStringLiteral("Futari Music · 正在安装客户端更新"));
}

void NotificationService::clearUpdateProgress() {
    m_tray.setIcon(m_baseIcon);
    m_tray.setToolTip(QStringLiteral("Futari Music"));
    m_lastProgress = -2;
    m_lastProgressIndeterminate = false;
}
