#include "nidaapp.h"
#include "systemtray.h"
#include "storageservice.h"
#include "apiservice.h"
#include "mediaservice.h"
#include "adhanscheduler.h"
#include "ui/mainwidget.h"
#include "ui/settingsdialog.h"
#include "ui/adhannotificationwindow.h"
#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDir>
#include <QScreen>
#include <QCursor>
#include <QDebug>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>

NidaApp::NidaApp(QObject *parent)
    : QObject(parent)
{
}

NidaApp::~NidaApp()
{
    delete m_widget;
    delete m_settings;
    delete m_notificationWindow;
}

void NidaApp::initialize()
{
    m_storage = new StorageService(this);
    if (!m_storage->initialize())
        qWarning() << "Storage init failed";

    m_api = new ApiService(this);
    m_media = new MediaService(this);
    m_scheduler = new AdhanScheduler(m_storage, m_media, this);

    m_tray = new SystemTray(this);
    m_widget = new MainWidget(m_storage);
    m_settings = nullptr;

    // Wire signals
    connect(m_tray, &SystemTray::showWidget,
            this, &NidaApp::onShowWidget);
    connect(m_widget, &MainWidget::settingsRequested,
            this, &NidaApp::onSettingsRequested);
    connect(m_scheduler, &AdhanScheduler::adhanStarted,
            this, &NidaApp::onAdhanStarted);
    connect(m_scheduler, &AdhanScheduler::prayerNotification,
            this, &NidaApp::onPrayerNotification);
    connect(m_scheduler, &AdhanScheduler::nextPrayerChanged,
            this, &NidaApp::onPrayerTimesUpdated);
    connect(m_api, &ApiService::prayerTimesFetched,
            this, &NidaApp::onPrayerTimesFetched);
    connect(m_api, &ApiService::fetchError,
            this, &NidaApp::onFetchError);

    // Show tray
    m_tray->show();
    setupAutoStart();

    // Load settings and start
    NidaSettings s = m_storage->loadSettings();
    m_scheduler->setVolume(s.volume);
    m_scheduler->setAdhanSound(s.selectedAdhan);

    // Try loading from cache first
    if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        m_times = m_storage->loadPrayerTimes(s.city, s.country, s.method);
        onPrayerTimesFetched(m_times);
    }

    // Fetch fresh times
    refreshPrayerTimes();

    // Set up IPC server for --notify command
    setupIpcServer();
    handleCommandLine();
}

void NidaApp::refreshPrayerTimes()
{
    NidaSettings s = m_storage->loadSettings();
    if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        DailyPrayerTimes cached = m_storage->loadPrayerTimes(s.city, s.country, s.method);
        m_scheduler->setPrayerTimes(cached);
        m_widget->setPrayerTimes(cached);
        m_scheduler->start();
    }
    m_api->fetchPrayerTimes(s.city, s.country, s.method);
}

void NidaApp::setupAutoStart()
{
    NidaSettings s = m_storage->loadSettings();
    if (!s.startupEnabled) return;

#ifdef Q_OS_LINUX
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                           + "/autostart";
    QDir().mkpath(autostartDir);
    QString desktopFile = autostartDir + "/nida.desktop";
    QFile f(desktopFile);
    if (f.open(QFile::WriteOnly)) {
        f.write(QString("[Desktop Entry]\n"
                        "Type=Application\n"
                        "Name=Nida\n"
                        "Exec=%1\n"
                        "Icon=nida\n"
                        "Terminal=false\n"
                        "X-GNOME-Autostart-enabled=true\n"
                        ).arg(QApplication::applicationFilePath()).toUtf8());
    }
#elif defined(Q_OS_WIN)
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    settings.setValue("Nida", QApplication::applicationFilePath());
#endif
}

void NidaApp::setupIpcServer()
{
    m_ipcServer = new QLocalServer(this);
    // Remove stale socket if the previous instance crashed
    QLocalServer::removeServer("nida-ipc");
    if (!m_ipcServer->listen("nida-ipc")) {
        qDebug() << "IPC server already running (another instance?)";
        return;
    }
    connect(m_ipcServer, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket *client = m_ipcServer->nextPendingConnection();
        if (!client) return;
        client->waitForReadyRead(500);
        QByteArray data = client->readAll();
        client->deleteLater();

        QString msg = QString::fromUtf8(data).trimmed();
        if (msg == "notify") {
            m_scheduler->triggerTestAdhan();
        }
    });
}

void NidaApp::handleCommandLine()
{
    QStringList args = QApplication::arguments();
    int idx = args.indexOf("--notify");
    if (idx < 0) return;

    // Try to connect to running instance
    QLocalSocket socket;
    socket.connectToServer("nida-ipc");
    if (socket.waitForConnected(500)) {
        socket.write("notify");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
    } else {
        qWarning() << "Nida is not running. Start Nida first, then use --notify.";
    }

    // If we're the second instance, quit after sending
    QApplication::quit();
}

void NidaApp::applyThemeToNotification()
{
    if (!m_notificationWindow) return;
    NidaSettings s = m_storage->loadSettings();
    QString theme = s.darkTheme ? "nida_dark" : "nida_light";
    QFile f(QString(":/styles/%1").arg(theme));
    if (f.open(QFile::ReadOnly))
        m_notificationWindow->setStyleSheet(f.readAll());
}

void NidaApp::showNotificationWindow(const QString &prayerName)
{
    if (!m_notificationWindow) {
        m_notificationWindow = new AdhanNotificationWindow;
        connect(m_notificationWindow, &AdhanNotificationWindow::dismissed,
                this, &NidaApp::onAdhanDismissed);
    }
    applyThemeToNotification();
    m_notificationWindow->showForPrayer(prayerName);
}

void NidaApp::onShowWidget()
{
    m_widget->setPrayerTimes(m_times);
    m_widget->adjustSize();

    // Position near the cursor (tray icon click)
    QPoint cursorPos = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(cursorPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect screenGeo = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    int x = cursorPos.x() - m_widget->width() / 2;
    int y = cursorPos.y() - m_widget->height() - 10;

    // Keep within screen bounds
    if (x < screenGeo.left()) x = screenGeo.left() + 4;
    if (x + m_widget->width() > screenGeo.right())
        x = screenGeo.right() - m_widget->width() - 4;
    if (y < screenGeo.top()) y = cursorPos.y() + 20;

    m_widget->move(x, y);
    m_widget->show();
    m_widget->raise();
    m_widget->activateWindow();
}

void NidaApp::onSettingsRequested()
{
    if (!m_settings) {
        m_settings = new SettingsDialog(m_storage);
        connect(m_settings, &SettingsDialog::settingsChanged,
                this, &NidaApp::onSettingsChanged);
    }
    m_settings->loadSettings();
    m_settings->exec();
}

void NidaApp::onAdhanStarted(const QString &prayerName)
{
    m_tray->setActiveIcon(true);
    showNotificationWindow(prayerName);
}

void NidaApp::onPrayerNotification(const QString &prayerName)
{
    // Silent notification (adhan toggle is off for this prayer)
    showNotificationWindow(prayerName);
}

void NidaApp::onAdhanDismissed()
{
    m_scheduler->dismissAdhan();
    m_tray->setActiveIcon(false);
    if (m_notificationWindow)
        m_notificationWindow->hide();
}

void NidaApp::onPrayerTimesUpdated(const QString &name, int secondsUntil)
{
    int h = secondsUntil / 3600;
    int m = (secondsUntil % 3600) / 60;
    QString tip = QString("Next: %1 in %2h %3m").arg(name).arg(h).arg(m);
    m_tray->setTooltip(tip);
    m_widget->setTimeUntilNext(name, secondsUntil);
}

void NidaApp::onPrayerTimesFetched(const DailyPrayerTimes &times)
{
    m_times = times;
    m_scheduler->setPrayerTimes(times);
    m_widget->setPrayerTimes(times);

    NidaSettings s = m_storage->loadSettings();
    m_storage->savePrayerTimes(s.city, s.country, s.method, times);

    m_scheduler->start();
    onPrayerTimesUpdated(m_scheduler->nextPrayerName(),
                         m_scheduler->secondsUntilNextPrayer());
}

void NidaApp::onFetchError(const QString &error)
{
    qWarning() << "API fetch error:" << error;
    // Fall back to cache
    NidaSettings s = m_storage->loadSettings();
    if (!m_scheduler->nextPrayerName().isEmpty()) return;
    if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        m_times = m_storage->loadPrayerTimes(s.city, s.country, s.method);
        onPrayerTimesFetched(m_times);
    }
}

void NidaApp::onSettingsChanged()
{
    NidaSettings s = m_storage->loadSettings();
    m_scheduler->setVolume(s.volume);
    m_scheduler->setAdhanSound(s.selectedAdhan);
    setupAutoStart();
    refreshPrayerTimes();
}
