#include "mpriscontroller.h"
#include <QDebug>
#include <QDBusConnectionInterface>

QStringList MprisController::listPlayers()
{
    QStringList players;
    auto *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return players;

    QDBusReply<QStringList> reply = bus->registeredServiceNames();
    if (!reply.isValid()) return players;

    for (const QString &name : reply.value()) {
        if (name.startsWith("org.mpris.MediaPlayer2.")) {
            players.append(name);
        }
    }
    return players;
}

QString MprisController::playbackStatus(const QString &service)
{
    QDBusInterface props(service, "/org/mpris/MediaPlayer2",
                         "org.freedesktop.DBus.Properties",
                         QDBusConnection::sessionBus());
    if (!props.isValid())
        return {};
    QDBusReply<QDBusVariant> reply = props.call(
        "Get", "org.mpris.MediaPlayer2.Player", "PlaybackStatus");
    if (!reply.isValid())
        return {};
    return reply.value().variant().toString();
}

void MprisController::sendPlaybackCommand(const QString &service, const QString &method)
{
    QDBusInterface iface(service, "/org/mpris/MediaPlayer2",
                         "org.mpris.MediaPlayer2.Player",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call(QDBus::NoBlock, method);
    }
}

QStringList MprisController::pauseAll()
{
    QStringList paused;
    const QStringList players = listPlayers();
    for (const QString &player : players) {
        // Only pause players that are actually playing; leave already-paused
        // players alone so we don't resume something the user paused.
        const QString status = playbackStatus(player);
        if (status.isEmpty() || status == QLatin1String("Playing")) {
            sendPlaybackCommand(player, "Pause");
            paused.append(player);
            qDebug() << "MPRIS: Paused" << player;
        }
    }
    return paused;
}

void MprisController::resumePlayers(const QStringList &players)
{
    for (const QString &player : players) {
        sendPlaybackCommand(player, "Play");
        qDebug() << "MPRIS: Resumed" << player;
    }
}

void MprisController::resumeAll()
{
    resumePlayers(listPlayers());
}
