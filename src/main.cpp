#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include "app/nidaapp.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Nida");
    app.setApplicationDisplayName("Nida - Prayer Times");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Nida");
    app.setWindowIcon(QIcon(":/icons/icon_colored"));

    // Don't quit when the popup widget closes — system-tray app
    app.setQuitOnLastWindowClosed(false);

    // Set Fusion style for consistent cross-platform look
    app.setStyle("Fusion");

    NidaApp nidaApp;
    nidaApp.initialize();

    return app.exec();
}
