#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>
#include <QTimer>

#include "src/AppController.h"
#include "src/ClientUpdateManager.h"
#include "src/NotificationService.h"

namespace {
void logMessage(QtMsgType, const QMessageLogContext&, const QString& message) {
    const QString path = qEnvironmentVariable("FUTARI_LOG_FILE");
    if (path.isEmpty()) return;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << message << Qt::endl;
    }
}
void applyPalette(bool dark) {
    QPalette palette;
    palette.setColor(QPalette::Window, dark ? QColor("#111319") : QColor("#f5f6f8"));
    palette.setColor(QPalette::Base, dark ? QColor("#252932") : QColor("#ffffff"));
    palette.setColor(QPalette::Button, dark ? QColor("#303541") : QColor("#e9ecf0"));
    palette.setColor(QPalette::Text, dark ? QColor("#f6f7fa") : QColor("#18202b"));
    palette.setColor(QPalette::WindowText, dark ? QColor("#f6f7fa") : QColor("#18202b"));
    palette.setColor(QPalette::ButtonText, dark ? QColor("#f6f7fa") : QColor("#18202b"));
    palette.setColor(QPalette::Highlight, QColor("#13ba78"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    QApplication::setPalette(palette);
}
}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    if (qEnvironmentVariableIsSet("FUTARI_LOG_FILE")) qInstallMessageHandler(logMessage);
    app.setOrganizationName("Futari");
    app.setApplicationName("FutariMusic");
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/futari_icon_1024.png")));
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setDefaultAlphaBuffer(true);

    AppController controller;
    ClientUpdateManager updates;
    updates.setServerUrl(controller.serverUrl());
    QObject::connect(&controller, &AppController::serverUrlChanged, &updates,
                     [&controller, &updates] { updates.setServerUrl(controller.serverUrl()); });
    applyPalette(controller.darkMode());
    QObject::connect(&controller, &AppController::darkModeChanged, &app,
                     [&controller] { applyPalette(controller.darkMode()); });
    NotificationService notifications;
    QObject::connect(&updates, &ClientUpdateManager::updateStarted, &notifications,
                     &NotificationService::showUpdateStarted);
    QObject::connect(&updates, &ClientUpdateManager::downloadProgressChanged, &notifications,
                     [&updates, &notifications] {
                         if (updates.downloading()) {
                             notifications.setUpdateProgress(updates.downloadProgress(),
                                                             updates.progressIndeterminate());
                         }
                     });
    QObject::connect(&updates, &ClientUpdateManager::downloadStateChanged, &notifications,
                     [&updates, &notifications] {
                         if (updates.installing())
                             notifications.showUpdateInstalling();
                         else if (!updates.downloading())
                             notifications.clearUpdateProgress();
                     });
    QObject::connect(&updates, &ClientUpdateManager::updateSucceeded, &notifications,
                     &NotificationService::showUpdateSucceeded);
    QObject::connect(&updates, &ClientUpdateManager::updateFailed, &notifications,
                     &NotificationService::showUpdateFailed);
    QObject::connect(&controller, &AppController::invitationArrived, &notifications,
                     &NotificationService::showInvitation);
    QObject::connect(&notifications, &NotificationService::activated, &controller,
                     &AppController::openMessagesRequested);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    engine.rootContext()->setContextProperty("updateManager", &updates);
    QObject::connect(
        &updates, &ClientUpdateManager::requestApplicationClose, &app, [&engine, &app] {
            if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0)))
                window->close();
            else
                app.quit();
        });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &updates,
                     &ClientUpdateManager::restartAfterQuit);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("FutariMusic", "Main");
    QTimer::singleShot(1500, &updates, [&updates] { updates.checkForUpdates(true); });
    const QString screenshotPath = qEnvironmentVariable("FUTARI_SCREENSHOT_FILE");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(1200, &engine, [&engine, screenshotPath] {
            if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0)))
                window->grabWindow().save(screenshotPath);
        });
    }

    return QApplication::exec();
}
