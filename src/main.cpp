#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QNetworkInformation>
#include <QTextStream>
#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif
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
    const bool cliFlag = args.contains("--version") || args.contains("-v")
        || args.contains("--help") || args.contains("-h")
        || args.contains("--test-adhan") || args.contains("--notify");
#ifdef Q_OS_WINDOWS
    // WIN32_EXECUTABLE has no console: attach to the parent console (cmd /
    // PowerShell) so CLI flags can print. If stdout is already a pipe or
    // file (Git Bash, "> out.txt"), keep it instead.
    if (cliFlag) {
        const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        const DWORD type = (h && h != INVALID_HANDLE_VALUE) ? GetFileType(h)
                                                            : FILE_TYPE_UNKNOWN;
        const bool redirected = type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK;
        if (!redirected && AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE *fp = nullptr;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        }
    }
#else
    Q_UNUSED(cliFlag);
#endif

    if (args.contains("--version") || args.contains("-v")) {
        QTextStream out(stdout);
        out << "Nida " << app.applicationVersion() << "\n";
        out.flush();
        return 0;
    }
    if (args.contains("--help") || args.contains("-h")) {
        QTextStream out(stdout);
        out << "Usage: Nida [options]\n"
               "  --test-adhan          Simulate a prayer time: after a delay, run exactly\n"
               "                        what happens at adhan time (pause media, play the\n"
               "                        adhan, show the popup, resume media when done).\n"
               "    --delay SECONDS     Delay before it fires (default 30, max 3600)\n"
               "    --prayer NAME       Fajr, Dhuhr, Asr, Maghrib or Isha\n"
               "                        (default: the next upcoming prayer)\n"
               "  --notify              Play a test adhan immediately\n"
               "  --version             Print the version\n"
               "  --help                Show this help\n"
               "Commands are forwarded to the running Nida if there is one.\n";
        out.flush();
        return 0;
    }
    const TestAdhanRequest test = parseTestAdhanArgs(args);
    if (!test.error.isEmpty()) {
        QTextStream err(stderr);
        err << "Nida: " << test.error << "\n";
        err.flush();
        return 2;
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
