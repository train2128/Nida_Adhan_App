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
#include <QTimer>
#include <QProcessEnvironment>
#include <QLocalServer>
#include <QLocalSocket>

NidaApp::NidaApp(QObject *parent)
    : QObject(parent)
{
}

NidaApp::~NidaApp()
{
    // Children of `this` are deleted automatically; only delete
    // parent-less top-level widgets.
    delete m_widget;
    delete m_settings;
    delete m_notificationWindow;
}

void NidaApp::initialize()
{
    // Single-instance: if another Nida is already listening, forward our
    // command to it and quit without creating any UI.
    const QStringList args = QApplication::arguments();
    const bool wantNotify = args.contains("--notify");
    if (tryForwardToRunningInstance(wantNotify ? QStringLiteral("notify")
                                               : QStringLiteral("show"))) {
        QTimer::singleShot(0, qApp, &QApplication::quit);
        return;
    }

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
    connect(m_tray, &SystemTray::quitRequested,
            qApp, &QApplication::quit);
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

    // Try loading from cache first (validate: old 1.0.x caches may hold
    // rows written by the buggy loader; isValid() filters those out).
    // Coords locations use the coords-keyed cache so they work offline too.
    if (s.hasCoords()) {
        if (m_storage->hasValidCacheForCoords(s.latitude, s.longitude, s.method)) {
            const DailyPrayerTimes cached = m_storage->loadPrayerTimesForCoords(
                s.latitude, s.longitude, s.method);
            if (cached.isValid())
                onPrayerTimesFetched(cached);
        }
    } else if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        const DailyPrayerTimes cached =
            m_storage->loadPrayerTimes(s.city, s.country, s.method);
        if (cached.isValid())
            onPrayerTimesFetched(cached);
    }

    // Fetch fresh times
    refreshPrayerTimes();

    // Set up IPC server for --notify / single-instance forwarding.
    setupIpcServer();

    // Primary instance launched directly with --notify: play test adhan locally.
    if (QApplication::arguments().contains("--notify"))
        QTimer::singleShot(0, this, [this]() { m_scheduler->triggerTestAdhan(); });
}

void NidaApp::refreshPrayerTimes()
{
    NidaSettings s = m_storage->loadSettings();
    if (s.hasCoords()) {
        if (m_storage->hasValidCacheForCoords(s.latitude, s.longitude, s.method)) {
            DailyPrayerTimes cached = m_storage->loadPrayerTimesForCoords(
                s.latitude, s.longitude, s.method);
            if (cached.isValid()) {
                m_scheduler->setPrayerTimes(cached);
                m_widget->setPrayerTimes(cached);
                m_scheduler->start();
            }
        }
        m_api->fetchPrayerTimesByCoords(s.latitude, s.longitude, s.method);
        return;
    }
    if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        DailyPrayerTimes cached = m_storage->loadPrayerTimes(s.city, s.country, s.method);
        if (cached.isValid()) {
            m_scheduler->setPrayerTimes(cached);
            m_widget->setPrayerTimes(cached);
            m_scheduler->start();
        }
    }
    m_api->fetchPrayerTimes(s.city, s.country, s.method);
}

void NidaApp::setupAutoStart()
{
    NidaSettings s = m_storage->loadSettings();

#ifdef Q_OS_LINUX
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                           + "/autostart";
    const QString desktopFile = autostartDir + "/nida.desktop";
    if (!s.startupEnabled) {
        QFile::remove(desktopFile);
        return;
    }
    QDir().mkpath(autostartDir);
    QFile f(desktopFile);
    if (f.open(QFile::WriteOnly)) {
        f.write(QString("[Desktop Entry]\n"
                        "Type=Application\n"
                        "Name=Nida\n"
                        "Exec=%1\n"
                        "Icon=nida\n"
                        "Terminal=false\n"
                        "X-GNOME-Autostart-enabled=true\n"
                        ).arg(launchPath()).toUtf8());
    }
#elif defined(Q_OS_WIN)
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    if (s.startupEnabled)
        settings.setValue("Nida", launchPath());
    else
        settings.remove("Nida");
#endif
}

QString NidaApp::launchPath() const
{
#ifdef Q_OS_LINUX
    // Inside an AppImage, applicationFilePath() is a /tmp/.mount_* path that
    // changes every launch. Use $APPIMAGE (the downloaded file) instead.
    const QString appImage = QProcessEnvironment::systemEnvironment().value("APPIMAGE");
    if (!appImage.isEmpty())
        return QStringLiteral("\"%1\"").arg(appImage);
#endif
    return QStringLiteral("\"%1\"").arg(QApplication::applicationFilePath());
}

bool NidaApp::tryForwardToRunningInstance(const QString &message)
{
    QLocalSocket socket;
    socket.connectToServer("nida-ipc");
    if (!socket.waitForConnected(400))
        return false;
    socket.write(message.toUtf8());
    socket.waitForBytesWritten(400);
    socket.disconnectFromServer();
    return true;
}

void NidaApp::setupIpcServer()
{
    if (m_ipcServer)
        return;
    // Only remove a stale socket when nobody is listening on it.
    {
        QLocalSocket probe;
        probe.connectToServer("nida-ipc");
        if (probe.waitForConnected(300))
            return; // live primary exists; initialize() already forwarded
        QLocalServer::removeServer("nida-ipc");
    }
    m_ipcServer = new QLocalServer(this);
    if (!m_ipcServer->listen("nida-ipc")) {
        qWarning() << "Could not start IPC server:" << m_ipcServer->errorString();
        return;
    }
    connect(m_ipcServer, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket *client = m_ipcServer->nextPendingConnection();
        if (!client) return;
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            const QString msg = QString::fromUtf8(client->readAll()).trimmed();
            if (msg == "notify" && m_scheduler) {
                m_scheduler->triggerTestAdhan();
            } else if (msg == "show") {
                onShowWidget();
            }
            client->deleteLater();
        });
        // Fallback for clients that wrote before we connected the signal.
        QTimer::singleShot(600, client, [client]() {
            if (client->bytesAvailable() > 0)
                client->readyRead();
            else
                client->deleteLater();
        });
    });
}

void NidaApp::handleCommandLine()
{
    // Kept for compatibility: forwarding now happens at the top of
    // initialize(). A primary instance launched with --notify plays a test
    // adhan there instead of quitting.
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
    if (!m_widget)
        return;
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
    if (m_tray)
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
    if (m_scheduler)
        m_scheduler->dismissAdhan();
    if (m_tray)
        m_tray->setActiveIcon(false);
    if (m_notificationWindow)
        m_notificationWindow->hide();
}

void NidaApp::onPrayerTimesUpdated(const QString &name, int secondsUntil)
{
    if (!m_tray || !m_widget)
        return;
    if (name.isEmpty() || secondsUntil <= 0) {
        m_tray->setTooltip(QStringLiteral("Nida - Prayer Times"));
        return;
    }
    int h = secondsUntil / 3600;
    int m = (secondsUntil % 3600) / 60;
    QString tip = QString("Next: %1 in %2h %3m").arg(name).arg(h).arg(m);
    m_tray->setTooltip(tip);
    m_widget->setTimeUntilNext(name, secondsUntil);
}

void NidaApp::onPrayerTimesFetched(const DailyPrayerTimes &times)
{
    if (!times.isValid()) {
        qWarning() << "Ignoring invalid prayer times";
        return;
    }
    if (!m_scheduler || !m_widget || !m_storage)
        return;
    m_times = times;
    m_scheduler->setPrayerTimes(times);
    m_widget->setPrayerTimes(times);

    NidaSettings s = m_storage->loadSettings();
    if (s.hasCoords())
        m_storage->savePrayerTimesForCoords(s.latitude, s.longitude, s.method, times);
    else
        m_storage->savePrayerTimes(s.city, s.country, s.method, times);

    m_scheduler->start();
    onPrayerTimesUpdated(m_scheduler->nextPrayerName(),
                         m_scheduler->secondsUntilNextPrayer());
}

void NidaApp::onFetchError(const QString &error)
{
    qWarning() << "API fetch error:" << error;
    // Fall back to cache only if we have nothing to show yet.
    // Works offline: both coords and legacy caches are in local SQLite.
    NidaSettings s = m_storage->loadSettings();
    if (!m_scheduler->nextPrayerName().isEmpty()) return;
    DailyPrayerTimes cached;
    if (s.hasCoords()) {
        if (!m_storage->hasValidCacheForCoords(s.latitude, s.longitude, s.method))
            return;
        cached = m_storage->loadPrayerTimesForCoords(s.latitude, s.longitude, s.method);
    } else {
        if (!m_storage->hasValidCache(s.city, s.country, s.method))
            return;
        cached = m_storage->loadPrayerTimes(s.city, s.country, s.method);
    }
    if (cached.isValid())
        onPrayerTimesFetched(cached);
}

void NidaApp::onSettingsChanged()
{
    NidaSettings s = m_storage->loadSettings();
    m_scheduler->setVolume(s.volume);
    m_scheduler->setAdhanSound(s.selectedAdhan);
    setupAutoStart();
    refreshPrayerTimes();
}
