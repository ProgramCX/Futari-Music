#include "NotificationService.h"

#include "AppController.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPainterPath>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace {
enum class TrayGlyph { Previous, Play, Pause, Next };

QIcon createTrayIcon(TrayGlyph glyph, const QColor& color) {
    constexpr qreal deviceScale = 2;
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(deviceScale);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    QPainterPath path;
    switch (glyph) {
        case TrayGlyph::Previous:
            path.moveTo(15, 5);
            path.lineTo(6, 12);
            path.lineTo(15, 19);
            path.closeSubpath();
            painter.drawRect(QRectF(4, 5, 2, 14));
            painter.drawPath(path);
            break;
        case TrayGlyph::Play:
            path.moveTo(7, 4.8);
            path.lineTo(19, 12);
            path.lineTo(7, 19.2);
            path.closeSubpath();
            painter.drawPath(path);
            break;
        case TrayGlyph::Pause:
            painter.drawRoundedRect(QRectF(6, 5, 4, 14), 1, 1);
            painter.drawRoundedRect(QRectF(14, 5, 4, 14), 1, 1);
            break;
        case TrayGlyph::Next:
            path.moveTo(9, 5);
            path.lineTo(18, 12);
            path.lineTo(9, 19);
            path.closeSubpath();
            painter.drawPath(path);
            painter.drawRect(QRectF(18, 5, 2, 14));
            break;
    }
    return QIcon(pixmap);
}

QToolButton* createTrayButton(QWidget* parent, TrayGlyph glyph, const QString& label) {
    auto* button = new QToolButton(parent);
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(createTrayIcon(glyph, QColor(QStringLiteral("#18202b"))));
    button->setIconSize(QSize(18, 18));
    button->setFixedSize(38, 38);
    button->setToolTip(label);
    button->setAccessibleName(label);
    return button;
}
}  // namespace

NotificationService::NotificationService(QObject* parent) : QObject(parent) {
    m_baseIcon = QApplication::windowIcon();
    if (m_baseIcon.isNull()) m_baseIcon = QIcon(QStringLiteral(":/icons/futari_icon_1024.png"));
    if (m_baseIcon.isNull()) m_baseIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    m_tray.setIcon(m_baseIcon);
    m_tray.setToolTip(QStringLiteral("Futari Music"));
    m_contextMenu = new QMenu;
    auto* playbackAction = new QWidgetAction(m_contextMenu);
    playbackAction->setDefaultWidget(createPlaybackPanel());
    m_contextMenu->addAction(playbackAction);
    m_contextMenu->addSeparator();
    QAction* showWindowAction = m_contextMenu->addAction(QStringLiteral("显示主界面"));
    connect(showWindowAction, &QAction::triggered, this,
            &NotificationService::showMainWindowRequested);
    QAction* exitAction = m_contextMenu->addAction(QStringLiteral("退出 Futari Music"));
    connect(exitAction, &QAction::triggered, this, &NotificationService::exitRequested);
    m_tray.setContextMenu(m_contextMenu);
    connect(&m_tray, &QSystemTrayIcon::messageClicked, this, &NotificationService::messageClicked);
    connect(&m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                    emit activated();
            });
    if (QSystemTrayIcon::isSystemTrayAvailable()) m_tray.show();
}

NotificationService::~NotificationService() {
    m_tray.setContextMenu(nullptr);
    delete m_contextMenu;
}

QWidget* NotificationService::createPlaybackPanel() {
    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("trayPanel"));
    panel->setAttribute(Qt::WA_StyledBackground);
    panel->setMinimumWidth(292);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("未在播放"), panel);
    m_titleLabel->setObjectName(QStringLiteral("trayTitle"));
    m_titleLabel->setMaximumWidth(264);
    m_artistLabel = new QLabel(QStringLiteral("Futari Music"), panel);
    m_artistLabel->setObjectName(QStringLiteral("trayArtist"));
    m_artistLabel->setMaximumWidth(264);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_artistLabel);

    auto* controls = new QHBoxLayout;
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(10);
    m_previousButton = createTrayButton(panel, TrayGlyph::Previous, QStringLiteral("上一首"));
    m_toggleButton = createTrayButton(panel, TrayGlyph::Play, QStringLiteral("播放"));
    m_nextButton = createTrayButton(panel, TrayGlyph::Next, QStringLiteral("下一首"));
    controls->addStretch();
    controls->addWidget(m_previousButton);
    controls->addWidget(m_toggleButton);
    controls->addWidget(m_nextButton);
    controls->addStretch();
    layout->addLayout(controls);

    auto* volumeRow = new QHBoxLayout;
    m_volumeSlider = new QSlider(Qt::Horizontal, panel);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setAccessibleName(QStringLiteral("音量"));
    m_volumeLabel = new QLabel(QStringLiteral("70%"), panel);
    m_volumeLabel->setMinimumWidth(36);
    m_volumeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    volumeRow->addWidget(new QLabel(QStringLiteral("音量"), panel));
    volumeRow->addWidget(m_volumeSlider, 1);
    volumeRow->addWidget(m_volumeLabel);
    layout->addLayout(volumeRow);
    return panel;
}

void NotificationService::setPlaybackController(AppController* controller) {
    m_controller = controller;
    connect(controller, &AppController::sessionChanged, this,
            &NotificationService::refreshPlaybackMenu);
    connect(controller, &AppController::roomStateChanged, this,
            &NotificationService::refreshPlaybackMenu);
    connect(controller, &AppController::darkModeChanged, this,
            [this, controller] { setDarkMode(controller->darkMode()); });
    connect(controller->player(), &AudioPlayer::songChanged, this,
            &NotificationService::refreshPlaybackMenu);
    connect(controller->player(), &AudioPlayer::playingChanged, this,
            &NotificationService::refreshPlaybackMenu);
    connect(controller->player(), &AudioPlayer::volumeChanged, this,
            &NotificationService::refreshPlaybackMenu);
    connect(m_previousButton, &QToolButton::clicked, controller, &AppController::previousSong);
    connect(m_toggleButton, &QToolButton::clicked, controller, &AppController::togglePlayback);
    connect(m_nextButton, &QToolButton::clicked, controller, &AppController::nextSong);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_volumeLabel->setText(QStringLiteral("%1%").arg(value));
        if (m_controller) m_controller->player()->setVolume(value / 100.0);
    });
    setDarkMode(controller->darkMode());
    refreshPlaybackMenu();
}

void NotificationService::setDarkMode(bool dark) {
    m_darkMode = dark;
    const QString background = dark ? QStringLiteral("#191c22") : QStringLiteral("#ffffff");
    const QString panel = dark ? QStringLiteral("#252932") : QStringLiteral("#f4f6f8");
    const QString text = dark ? QStringLiteral("#f6f7fa") : QStringLiteral("#18202b");
    const QString muted = dark ? QStringLiteral("#9da6b5") : QStringLiteral("#697386");
    const QString border = dark ? QStringLiteral("#363c47") : QStringLiteral("#e3e7ed");
    const QString hover = dark ? QStringLiteral("#303641") : QStringLiteral("#e9edf2");
    m_contextMenu->setStyleSheet(QStringLiteral(
        "QMenu { background:%1; color:%2; border:1px solid %5; border-radius:14px; padding:6px; }"
        "QMenu::item { color:%2; padding:9px 16px; margin:2px; border-radius:8px; }"
        "QMenu::item:selected { background:%6; }"
        "QMenu::separator { height:1px; margin:5px 12px; background:%5; }"
        "QWidget#trayPanel { background:%3; border-radius:11px; }"
        "QLabel#trayTitle { color:%2; font-size:14px; font-weight:600; }"
        "QLabel#trayArtist { color:%4; font-size:12px; }"
        "QLabel { color:%4; font-size:12px; }"
        "QToolButton { color:%2; background:transparent; border:0; "
        "border-radius:19px; padding:6px; }"
        "QToolButton:hover { background:%6; } QToolButton:pressed { background:%5; }"
        "QSlider::groove:horizontal { height:4px; border-radius:2px; background:%5; }"
        "QSlider::sub-page:horizontal { border-radius:2px; background:#13ba78; }"
        "QSlider::handle:horizontal { width:12px; height:12px; margin:-5px 0; "
        "border-radius:6px; background:#13ba78; }")
        .arg(background, text, panel, muted, border, hover));
    refreshPlaybackIcons();
}

void NotificationService::refreshPlaybackMenu() {
    if (!m_controller) return;
    const QVariantMap song = m_controller->player()->song();
    const bool hasSong = !song.isEmpty();
    const QString title = hasSong ? song.value("title").toString() : QStringLiteral("未在播放");
    const QString artist =
        hasSong ? song.value("artist").toString() : QStringLiteral("Futari Music");
    m_titleLabel->setText(
        QFontMetrics(m_titleLabel->font()).elidedText(title, Qt::ElideRight, 260));
    m_titleLabel->setToolTip(title);
    m_artistLabel->setText(
        QFontMetrics(m_artistLabel->font()).elidedText(artist, Qt::ElideRight, 260));
    m_artistLabel->setToolTip(artist);
    const bool canControl = m_controller->canControl();
    m_previousButton->setEnabled(hasSong && canControl);
    m_toggleButton->setEnabled(hasSong && canControl);
    const QVariantMap roomState = m_controller->roomState();
    const bool inRoom = !roomState.value("room").toMap().isEmpty();
    const int queueSize = inRoom ? roomState.value("songIds").toList().size()
                                 : m_controller->localQueue().size();
    m_nextButton->setEnabled(canControl && queueSize > 0);
    const bool playing = m_controller->player()->playing();
    m_toggleButton->setProperty("playing", playing);
    m_toggleButton->setToolTip(playing ? QStringLiteral("暂停") : QStringLiteral("播放"));
    m_toggleButton->setAccessibleName(m_toggleButton->toolTip());
    const int volume = qRound(m_controller->player()->volume() * 100.0);
    const QSignalBlocker blocker(m_volumeSlider);
    m_volumeSlider->setValue(volume);
    m_volumeLabel->setText(QStringLiteral("%1%").arg(volume));
    refreshPlaybackIcons();
}

void NotificationService::refreshPlaybackIcons() {
    const QColor foreground = m_darkMode ? QColor(QStringLiteral("#f6f7fa"))
                                         : QColor(QStringLiteral("#18202b"));
    const QColor disabled = m_darkMode ? QColor(QStringLiteral("#626975"))
                                       : QColor(QStringLiteral("#a5adb8"));
    m_previousButton->setIcon(createTrayIcon(
        TrayGlyph::Previous, m_previousButton->isEnabled() ? foreground : disabled));
    m_toggleButton->setIcon(createTrayIcon(
        m_toggleButton->property("playing").toBool() ? TrayGlyph::Pause : TrayGlyph::Play,
        m_toggleButton->isEnabled() ? foreground : disabled));
    m_nextButton->setIcon(createTrayIcon(
        TrayGlyph::Next, m_nextButton->isEnabled() ? foreground : disabled));
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
