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
    void setAdhanPlaying(bool playing); // shows/hides "Stop adhan"
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

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_nextPrayerAction = nullptr;
    QAction *m_showAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_quitAction = nullptr;
};

#endif
