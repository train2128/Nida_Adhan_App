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
    // Tray icon: green normally, orange in the last minutes before a prayer,
    // blue while the adhan plays. Playing wins over "soon".
    void setAdhanPlaying(bool playing); // also shows/hides "Stop adhan"
    void setPrayerSoon(bool soon);
    void show();
    void showMessage(const QString &title, const QString &message);

signals:
    void clicked();
    void showWidget();
    void quitRequested();
    void stopAdhanRequested();

private slots:
    void onIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupMenu();
    void updateIcon();

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_nextPrayerAction = nullptr;
    QAction *m_showAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_quitAction = nullptr;
    QIcon m_iconNormal;
    QIcon m_iconSoon;
    QIcon m_iconPlaying;
    bool m_playing = false;
    bool m_soon = false;
};

#endif
