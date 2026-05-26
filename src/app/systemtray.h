#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>

class SystemTray : public QObject
{
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setTooltip(const QString &tooltip);
    void setActiveIcon(bool active);
    void show();

signals:
    void clicked();
    void showWidget();
    void quitRequested();

private slots:
    void onIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupMenu();

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QAction *m_nextPrayerAction;
    QAction *m_showAction;
    QAction *m_quitAction;
};

#endif
