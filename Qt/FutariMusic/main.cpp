#include "src/AppController.h"
#include "src/NotificationService.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QTextStream>
#include <QQuickStyle>
#include <QPalette>
#include <QQuickWindow>
#include <QTimer>

namespace {
void logMessage(QtMsgType, const QMessageLogContext &, const QString &message) {
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
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (qEnvironmentVariableIsSet("FUTARI_LOG_FILE")) qInstallMessageHandler(logMessage);
    app.setOrganizationName("Futari");
    app.setApplicationName("FutariMusic");
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setDefaultAlphaBuffer(true);

    AppController controller;
    applyPalette(controller.darkMode());
    QObject::connect(&controller, &AppController::darkModeChanged, &app,
                     [&controller] { applyPalette(controller.darkMode()); });
    NotificationService notifications;
    QObject::connect(&controller, &AppController::invitationArrived,
                     &notifications, &NotificationService::showInvitation);
    QObject::connect(&notifications, &NotificationService::activated,
                     &controller, &AppController::openMessagesRequested);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("FutariMusic", "Main");
    const QString screenshotPath = qEnvironmentVariable("FUTARI_SCREENSHOT_FILE");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(1200, &engine, [&engine, screenshotPath] {
            if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0)))
                window->grabWindow().save(screenshotPath);
        });
    }

    return QApplication::exec();
}
