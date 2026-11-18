#include "systemtray.h"
#include <QIcon>
#include <QDebug>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu(nullptr))
    , m_iconNormal(QStringLiteral(":/icons/nida_icon"))
    , m_iconSoon(QStringLiteral(":/icons/nida_icon_soon"))
    , m_iconPlaying(QStringLiteral(":/icons/nida_icon_playing"))
{
    m_trayIcon->setIcon(m_iconNormal);
    m_trayIcon->setToolTip("Nida - Prayer Times");

    setupMenu();
    m_trayIcon->setContextMenu(m_menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &SystemTray::onIconActivated);
    connect(m_showAction, &QAction::triggered, this, &SystemTray::showWidget);
    connect(m_quitAction, &QAction::triggered, this, &SystemTray::quitRequested);
    connect(m_stopAction, &QAction::triggered, this, &SystemTray::stopAdhanRequested);
}

SystemTray::~SystemTray()
{
    delete m_menu;
}

void SystemTray::setupMenu()
{
    m_nextPrayerAction = m_menu->addAction("Loading...");
    m_nextPrayerAction->setEnabled(false);
    m_menu->addSeparator();
    m_stopAction = m_menu->addAction("Stop adhan");
    m_stopAction->setVisible(false);
    m_showAction = m_menu->addAction("Show prayer times");
    m_quitAction = m_menu->addAction("Quit");
}

void SystemTray::setTooltip(const QString &tooltip)
{
    m_trayIcon->setToolTip(tooltip);
    m_nextPrayerAction->setText(tooltip);
}

void SystemTray::setAdhanPlaying(bool playing)
{
    m_playing = playing;
    m_stopAction->setVisible(playing);
    updateIcon();
}

void SystemTray::setPrayerSoon(bool soon)
{
    m_soon = soon;
    updateIcon();
}

void SystemTray::updateIcon()
{
    // Always derived from the current state, so the icon can never get stuck
    // (the old code switched to a black icon after the adhan and never back).
    const QIcon &icon = m_playing ? m_iconPlaying : (m_soon ? m_iconSoon : m_iconNormal);
    if (m_trayIcon->icon().cacheKey() != icon.cacheKey())
        m_trayIcon->setIcon(icon);
}

void SystemTray::show()
{
    m_trayIcon->show();
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        qWarning() << "System tray not available - tray icon may be invisible"
                      " (install AppIndicator extension on GNOME/Wayland)";
}

void SystemTray::showMessage(const QString &title, const QString &message)
{
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 8000);
}

void SystemTray::onIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        emit showWidget();
}
