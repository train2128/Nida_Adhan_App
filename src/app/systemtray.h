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
    ~SystemTray() override;
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

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_nextPrayerAction = nullptr;
    QAction *m_showAction = nullptr;
    QAction *m_quitAction = nullptr;
};

#endif
