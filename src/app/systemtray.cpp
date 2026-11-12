#include "systemtray.h"
#include <QIcon>
#include <QDebug>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu(nullptr))
{
    m_trayIcon->setIcon(QIcon(":/icons/icon_colored"));
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

void SystemTray::setActiveIcon(bool active)
{
    Q_UNUSED(active);
    // The colored icon is the normal one (as at startup). Switching to the
    // black icon after the first adhan made the tray look different forever.
    m_trayIcon->setIcon(QIcon(":/icons/icon_colored"));
}

void SystemTray::setAdhanPlaying(bool playing)
{
    m_stopAction->setVisible(playing);
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
