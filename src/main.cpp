#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QDebug>
#include "app/nidaapp.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Nida");
    app.setApplicationDisplayName("Nida - نداء");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Nida");
    app.setWindowIcon(QIcon(":/icons/icon_colored"));

    // Set Fusion style for consistent cross-platform look
    app.setStyle("Fusion");

    NidaApp nidaApp;
    nidaApp.initialize();

    return app.exec();
}
