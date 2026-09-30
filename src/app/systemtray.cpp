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
    m_showAction = m_menu->addAction("Show Prayers");
    m_quitAction = m_menu->addAction("Quit");
}

void SystemTray::setTooltip(const QString &tooltip)
{
    m_trayIcon->setToolTip(tooltip);
    m_nextPrayerAction->setText(tooltip);
}

void SystemTray::setActiveIcon(bool active)
{
    if (active)
        m_trayIcon->setIcon(QIcon(":/icons/icon_colored"));
    else
        m_trayIcon->setIcon(QIcon(":/icons/icon_black"));
}

void SystemTray::show()
{
    m_trayIcon->show();
}

void SystemTray::onIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        emit showWidget();
}
