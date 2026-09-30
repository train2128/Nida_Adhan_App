#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QNetworkInformation>
#include <QTextStream>
#include "app/nidaapp.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Nida");
    app.setApplicationDisplayName("Nida - Prayer Times");
    app.setApplicationVersion("1.1.0");
    app.setOrganizationName("Nida");
    app.setOrganizationDomain("github.com/train2128/Nida_Adhan_App");
    app.setWindowIcon(QIcon(":/icons/icon_colored"));

    const QStringList args = app.arguments();
    if (args.contains("--version") || args.contains("-v")) {
        QTextStream out(stdout);
        out << "Nida " << app.applicationVersion() << "\n";
        return 0;
    }
    if (args.contains("--help") || args.contains("-h")) {
        QTextStream out(stdout);
        out << "Usage: Nida [--notify] [--version] [--help]\n"
               "  --notify   Play a test adhan (forwards to running instance if any)\n";
        return 0;
    }

    // Enable QNetworkInformation reachability backend when available.
    QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability);

    // Don't quit when the popup widget closes — system-tray app
    app.setQuitOnLastWindowClosed(false);

    // Set Fusion style for consistent cross-platform look
    app.setStyle("Fusion");

    NidaApp nidaApp;
    nidaApp.initialize();

    return app.exec();
}
