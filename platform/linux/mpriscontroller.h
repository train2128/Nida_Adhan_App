#ifndef MPRISCONTROLLER_H
#define MPRISCONTROLLER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QStringList>

class MprisController : public QObject
{
    Q_OBJECT
public:
    // Pauses only players that are currently Playing and returns them,
    // so callers resume exactly what was paused.
    static QStringList pauseAll();
    static void resumeAll();
    static void resumePlayers(const QStringList &players);

private:
    static QStringList listPlayers();
    static QString playbackStatus(const QString &service);
    static void sendPlaybackCommand(const QString &service, const QString &method);
};

#endif
