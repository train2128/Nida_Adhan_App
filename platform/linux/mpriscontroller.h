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
    static void pauseAll();
    static void resumeAll();

private:
    static QStringList listPlayers();
    static void sendPlaybackCommand(const QString &service, const QString &method);
};

#endif
