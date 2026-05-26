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

void MprisController::sendPlaybackCommand(const QString &service, const QString &method)
{
    QDBusInterface iface(service, "/org/mpris/MediaPlayer2",
                         "org.mpris.MediaPlayer2.Player",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call(QDBus::NoBlock, method);
    }
}

void MprisController::pauseAll()
{
    QStringList players = listPlayers();
    for (const QString &player : players) {
        sendPlaybackCommand(player, "Pause");
        qDebug() << "MPRIS: Paused" << player;
    }
}

void MprisController::resumeAll()
{
    QStringList players = listPlayers();
    for (const QString &player : players) {
        sendPlaybackCommand(player, "Play");
        qDebug() << "MPRIS: Resumed" << player;
    }
}
