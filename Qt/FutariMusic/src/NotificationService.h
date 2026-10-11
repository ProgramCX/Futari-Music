#pragma once

#include <QIcon>
#include <QObject>
#include <QSystemTrayIcon>

class AppController;
class QLabel;
class QMenu;
class QSlider;
class QToolButton;
class QWidget;

class NotificationService final : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject* parent = nullptr);
    ~NotificationService() override;
    void setPlaybackController(AppController* controller);
    void setDarkMode(bool dark);
    bool available() const { return QSystemTrayIcon::isSystemTrayAvailable(); }
    void showInvitation(const QString& title, const QString& message);
    void showUpdateStarted(const QString& release_tag);
    void showUpdateSucceeded(const QString& release_tag);
    void showUpdateFailed(const QString& message);
    void setUpdateProgress(int percentage, bool indeterminate);
    void showUpdateInstalling();
    void clearUpdateProgress();
signals:
    void activated();
    void messageClicked();
    void showMainWindowRequested();
    void exitRequested();

private:
    QWidget* createPlaybackPanel();
    void refreshPlaybackMenu();
    void refreshPlaybackIcons();
    QIcon m_baseIcon;
    QSystemTrayIcon m_tray;
    AppController* m_controller = nullptr;
    QMenu* m_contextMenu = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_artistLabel = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QToolButton* m_previousButton = nullptr;
    QToolButton* m_toggleButton = nullptr;
    QToolButton* m_nextButton = nullptr;
    int m_lastProgress = -2;
    bool m_lastProgressIndeterminate = false;
    bool m_darkMode = false;
};
