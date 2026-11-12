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
#include <QStyleHints>
#include "ui/theme.h"
#include <QNetworkInformation>
#include <QTextStream>

namespace {
const QStringList kPrayerNames = {
    QStringLiteral("Fajr"), QStringLiteral("Dhuhr"), QStringLiteral("Asr"),
    QStringLiteral("Maghrib"), QStringLiteral("Isha")
};
}

TestAdhanRequest parseTestAdhanArgs(const QStringList &args)
{
    TestAdhanRequest r;
    r.requested = args.contains(QStringLiteral("--test-adhan"));
    if (!r.requested)
        return r;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == QLatin1String("--delay")) {
            bool ok = false;
            const int v = (i + 1 < args.size()) ? args.at(++i).toInt(&ok) : 0;
            if (!ok || v < 0 || v > 3600) {
                r.error = QStringLiteral("--delay expects a number of seconds (0-3600)");
                return r;
            }
            r.delaySec = v;
        } else if (a == QLatin1String("--prayer")) {
            const QString name = (i + 1 < args.size()) ? args.at(++i) : QString();
            r.prayer.clear();
            for (const QString &p : kPrayerNames) {
                if (p.compare(name, Qt::CaseInsensitive) == 0)
                    r.prayer = p;
            }
            if (r.prayer.isEmpty()) {
                r.error = QStringLiteral("--prayer expects one of: ") + kPrayerNames.join(", ");
                return r;
            }
        }
    }
    return r;
}

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
    const TestAdhanRequest test = parseTestAdhanArgs(args);
    const QString testWho = test.prayer.isEmpty() ? QStringLiteral("the next prayer")
                                                  : test.prayer;
    QString message = QStringLiteral("show");
    if (test.requested)
        message = QStringLiteral("test-adhan %1 %2").arg(test.delaySec).arg(test.prayer);
    else if (wantNotify)
        message = QStringLiteral("notify");
    if (tryForwardToRunningInstance(message)) {
        if (test.requested) {
            QTextStream(stdout) << "Nida: simulating adhan time for " << testWho
                                << " in " << test.delaySec << " s (sent to running Nida).\n"
                                << "Start some music/video now to check it gets paused"
                                   " and resumed.\n";
        }
        QTimer::singleShot(0, qApp, &QApplication::quit);
        return;
    }
    if (test.requested) {
        QTextStream(stdout) << "Nida: no running instance, starting it; simulating adhan time for "
                            << testWho << " in " << test.delaySec << " s.\n";
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
    connect(m_tray, &SystemTray::stopAdhanRequested,
            this, &NidaApp::onStopAdhan);
    connect(m_widget, &MainWidget::refreshRequested, this, [this]() {
        m_retryTimer->stop();
        m_retryDelaySec = 0;
        refreshPrayerTimes();
    });
    connect(m_scheduler, &AdhanScheduler::adhanFinished,
            this, &NidaApp::onAdhanFinished);
    connect(m_scheduler, &AdhanScheduler::playbackProgress, this,
            [this](qint64 pos, qint64 dur) {
                if (m_notificationWindow && m_notificationWindow->isVisible())
                    m_notificationWindow->setProgress(pos, dur);
            });
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // "Match system" theme follows the OS light/dark switch live.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &NidaApp::applyTheme);
#endif
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
    connect(m_scheduler, &AdhanScheduler::dateChanged,
            this, &NidaApp::onDateChanged);

    m_testTimer = new QTimer(this);
    m_testTimer->setSingleShot(true);
    connect(m_testTimer, &QTimer::timeout, this, &NidaApp::onTestAdhanTimeout);

    // Retry failed fetches (e.g. network not up yet at login) with backoff.
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &NidaApp::onRetryFetch);

    // Retry immediately when connectivity comes back.
    if (auto *netInfo = QNetworkInformation::instance()) {
        connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
                [this](QNetworkInformation::Reachability r) {
                    if (r == QNetworkInformation::Reachability::Online
                        && m_retryTimer->isActive()) {
                        m_retryTimer->stop();
                        onRetryFetch();
                    }
                });
    }

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
                applyPrayerTimes(cached);
        }
    } else if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        const DailyPrayerTimes cached =
            m_storage->loadPrayerTimes(s.city, s.country, s.method);
        if (cached.isValid())
            applyPrayerTimes(cached);
    }

    // Fetch fresh times
    refreshPrayerTimes();

    // Set up IPC server for --notify / single-instance forwarding.
    setupIpcServer();

    // Primary instance launched directly with --test-adhan / --notify.
    if (test.requested)
        scheduleTestAdhan(test.delaySec, test.prayer);
    else if (wantNotify)
        QTimer::singleShot(0, this, [this]() { m_scheduler->triggerTestAdhan(); });
}

void NidaApp::scheduleTestAdhan(int delaySec, const QString &prayer)
{
    m_testPrayer = prayer;
    const QString who = prayer.isEmpty() ? QStringLiteral("the next prayer") : prayer;
    qInfo() << "Test adhan for" << who << "in" << delaySec << "s";
    if (m_tray) {
        m_tray->showMessage(QStringLiteral("Nida - test adhan"),
                            QStringLiteral("Simulating adhan time for %1 in %2 seconds.")
                                .arg(who).arg(delaySec));
    }
    m_testTimer->start(delaySec * 1000);
}

void NidaApp::onTestAdhanTimeout()
{
    QString prayer = m_testPrayer;
    if (prayer.isEmpty()) {
        prayer = m_scheduler->nextPrayerName();
        if (prayer.isEmpty() || prayer == QLatin1String("Sunrise"))
            prayer = QStringLiteral("Dhuhr"); // no times loaded yet / no adhan at sunrise
    }
    // Clear any adhan still playing so the test behaves like a fresh prayer.
    m_scheduler->dismissAdhan();
    if (m_notificationWindow)
        m_notificationWindow->hide();
    if (!m_storage->isAdhanEnabled(prayer)) {
        qInfo() << "Adhan is turned off for" << prayer << "- showing silent notification";
        if (m_tray) {
            m_tray->showMessage(QStringLiteral("Nida - test adhan"),
                                QStringLiteral("Adhan sound is turned off for %1, so only the "
                                               "notification is shown.").arg(prayer));
        }
    }
    m_scheduler->announcePrayer(prayer);
}

void NidaApp::refreshPrayerTimes()
{
    NidaSettings s = m_storage->loadSettings();
    if (s.hasCoords()) {
        if (m_storage->hasValidCacheForCoords(s.latitude, s.longitude, s.method)) {
            DailyPrayerTimes cached = m_storage->loadPrayerTimesForCoords(
                s.latitude, s.longitude, s.method);
            if (cached.isValid())
                applyPrayerTimes(cached);
        }
        m_api->fetchPrayerTimesByCoords(s.latitude, s.longitude, s.method);
        return;
    }
    if (m_storage->hasValidCache(s.city, s.country, s.method)) {
        DailyPrayerTimes cached = m_storage->loadPrayerTimes(s.city, s.country, s.method);
        if (cached.isValid())
            applyPrayerTimes(cached);
    }
    m_api->fetchPrayerTimes(s.city, s.country, s.method);
}

void NidaApp::setupAutoStart()
{
    NidaSettings s = m_storage->loadSettings();

#if defined(Q_OS_LINUX)
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
#elif defined(Q_OS_WINDOWS) || defined(Q_OS_WIN)
    // Windows autostart via HKCU Run (per-user, no admin needed).
    // launchPath() returns a quoted path so dirs with spaces work.
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    if (s.startupEnabled)
        settings.setValue("Nida", launchPath());
    else
        settings.remove("Nida");
#else
    qWarning() << "Autostart not implemented on this platform";
#endif
}

QString NidaApp::launchPath() const
{
#if defined(Q_OS_LINUX)
    // Inside an AppImage, applicationFilePath() is a /tmp/.mount_* path that
    // changes every launch. Use $APPIMAGE (the downloaded file) instead.
    const QString appImage = QProcessEnvironment::systemEnvironment().value("APPIMAGE");
    if (!appImage.isEmpty())
        return QStringLiteral("\"%1\"").arg(appImage);
#endif
    return QStringLiteral("\"%1\"").arg(
        QDir::toNativeSeparators(QApplication::applicationFilePath()));
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
            } else if (msg.startsWith(QLatin1String("test-adhan")) && m_scheduler) {
                // "test-adhan <delaySec> [Prayer]" (already validated by sender)
                const QStringList parts = msg.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                const int delay = parts.value(1).toInt();
                const QString prayer = parts.value(2);
                if (prayer.isEmpty() || kPrayerNames.contains(prayer))
                    scheduleTestAdhan(qBound(0, delay, 3600), prayer);
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
    m_notificationWindow->setDark(Theme::isDark(m_storage->loadSettings()));
}

void NidaApp::applyTheme()
{
    if (m_widget)
        m_widget->applyTheme();
    applyThemeToNotification();
}

void NidaApp::showNotificationWindow(const QString &prayerName, bool withAdhan)
{
    if (!m_notificationWindow) {
        m_notificationWindow = new AdhanNotificationWindow;
        connect(m_notificationWindow, &AdhanNotificationWindow::closed,
                this, &NidaApp::onAdhanDismissed);
        connect(m_notificationWindow, &AdhanNotificationWindow::stopRequested,
                this, &NidaApp::onStopAdhan);
    }
    applyThemeToNotification();
    m_notificationWindow->showForPrayer(prayerName, m_scheduler->prayerTime(prayerName),
                                        withAdhan, m_scheduler->mediaPaused());
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
        m_tray->setAdhanPlaying(true);
    showNotificationWindow(prayerName, true);
}

void NidaApp::onPrayerNotification(const QString &prayerName)
{
    // Silent notification (adhan toggle is off for this prayer)
    showNotificationWindow(prayerName, false);
}

void NidaApp::onAdhanDismissed()
{
    // Popup closed with "Close": hide only; the adhan keeps playing and can
    // still be stopped from the tray menu.
    if (m_notificationWindow)
        m_notificationWindow->hide();
}

void NidaApp::onStopAdhan()
{
    if (m_scheduler)
        m_scheduler->dismissAdhan(); // emits adhanFinished -> onAdhanFinished
}

void NidaApp::onAdhanFinished()
{
    if (m_tray) {
        m_tray->setAdhanPlaying(false);
        m_tray->setActiveIcon(false);
    }
    if (m_notificationWindow && m_notificationWindow->isVisible())
        m_notificationWindow->setAdhanFinished();
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
    // A successful answer ends any retry cycle.
    m_retryTimer->stop();
    m_retryDelaySec = 0;

    NidaSettings s = m_storage->loadSettings();
    if (s.hasCoords())
        m_storage->savePrayerTimesForCoords(s.latitude, s.longitude, s.method, times);
    else
        m_storage->savePrayerTimes(s.city, s.country, s.method, times);

    applyPrayerTimes(times);
}

void NidaApp::applyPrayerTimes(const DailyPrayerTimes &times)
{
    m_times = times;
    m_scheduler->setPrayerTimes(times);
    m_widget->setPrayerTimes(times);
    m_scheduler->start();
    onPrayerTimesUpdated(m_scheduler->nextPrayerName(),
                         m_scheduler->secondsUntilNextPrayer());
}

void NidaApp::scheduleRetry()
{
    // 30s, 60s, 2m, 4m, ... capped at 15 minutes.
    m_retryDelaySec = m_retryDelaySec <= 0 ? 30 : qMin(m_retryDelaySec * 2, 15 * 60);
    qInfo() << "Retrying prayer-times fetch in" << m_retryDelaySec << "s";
    m_retryTimer->start(m_retryDelaySec * 1000);
}

void NidaApp::onRetryFetch()
{
    refreshPrayerTimes();
}

void NidaApp::onDateChanged()
{
    // New day (midnight or wake from sleep): load/fetch today's times.
    m_retryTimer->stop();
    m_retryDelaySec = 0;
    refreshPrayerTimes();
}

void NidaApp::onFetchError(const QString &error)
{
    qWarning() << "API fetch error:" << error;
    // Keep trying until we get today's times (the cache, if any, is in use
    // meanwhile). Stops as soon as one fetch succeeds.
    scheduleRetry();

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
        applyPrayerTimes(cached); // keep the retry timer running
}

void NidaApp::onSettingsChanged()
{
    NidaSettings s = m_storage->loadSettings();
    m_scheduler->setVolume(s.volume);
    m_scheduler->setAdhanSound(s.selectedAdhan);
    setupAutoStart();
    applyTheme();
    m_retryTimer->stop();
    m_retryDelaySec = 0;
    refreshPrayerTimes();
}
