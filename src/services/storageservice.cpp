#include "storageservice.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QTime>

StorageService::StorageService(QObject *parent)
    : QObject(parent)
{
}

StorageService::~StorageService()
{
    if (m_db.isOpen())
        m_db.close();
}

bool StorageService::initialize()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/nida.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    createTables();
    return true;
}

void StorageService::createTables()
{
    QSqlQuery query(m_db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS prayer_cache ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  date TEXT NOT NULL,"
        "  city TEXT NOT NULL,"
        "  country TEXT NOT NULL,"
        "  method INTEGER NOT NULL,"
        "  hijri_day TEXT, hijri_month_ar TEXT, hijri_year TEXT,"
        "  fajr TEXT, sunrise TEXT, dhuhr TEXT, asr TEXT, maghrib TEXT, isha TEXT,"
        "  fetched_at TEXT NOT NULL,"
        "  UNIQUE(date, city, country, method)"
        ")"
    );
    query.exec(
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT"
        ")"
    );
    query.exec(
        "CREATE TABLE IF NOT EXISTS adhan_toggles ("
        "  prayer_name TEXT PRIMARY KEY,"
        "  enabled INTEGER NOT NULL DEFAULT 1"
        ")"
    );

    QSqlQuery init(m_db);
    init.exec("INSERT OR IGNORE INTO settings (key, value) VALUES "
              "('city', 'Mecca'),"
              "('country', 'Saudi Arabia'),"
              "('method', '3'),"
              "('selected_adhan', 'Adhan-Makkah'),"
              "('volume', '80'),"
              "('startup_enabled', 'true'),"
              "('dark_theme', 'true'),"
              "('language', 'en')");
}

bool StorageService::hasValidCache(const QString &city, const QString &country, int method)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT fetched_at FROM prayer_cache "
                  "WHERE date = :date AND city = :city AND country = :country AND method = :method");
    query.bindValue(":date", QDate::currentDate().toString("yyyy-MM-dd"));
    query.bindValue(":city", city);
    query.bindValue(":country", country);
    query.bindValue(":method", method);
    if (query.exec() && query.next()) {
        QDateTime fetched = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
        return fetched.isValid() && fetched.secsTo(QDateTime::currentDateTime()) < 86400;
    }
    return false;
}

DailyPrayerTimes StorageService::loadPrayerTimes(const QString &city, const QString &country, int method)
{
    DailyPrayerTimes times;
    QSqlQuery query(m_db);
    query.prepare("SELECT hijri_day, hijri_month_ar, hijri_year, fajr, sunrise, dhuhr, asr, maghrib, isha "
                  "FROM prayer_cache WHERE date = :date AND city = :city "
                  "AND country = :country AND method = :method");
    query.bindValue(":date", QDate::currentDate().toString("yyyy-MM-dd"));
    query.bindValue(":city", city);
    query.bindValue(":country", country);
    query.bindValue(":method", method);
    if (query.exec() && query.next()) {
        times.miladiDate = QDate::currentDate();
        times.hijriDay = query.value(0).toString();
        times.hijriMonthAr = query.value(1).toString();
        times.hijriYear = query.value(2).toString();
        times.fajr.name = "Fajr"; times.fajr.nameAr = "الفجر";
        times.fajr.time = QTime::fromString(query.value(3).toString(), "HH:mm");
        times.sunrise.name = "Sunrise"; times.sunrise.nameAr = "الشروق";
        times.sunrise.time = QTime::fromString(query.value(4).toString(), "HH:mm");
        times.dhuhr.name = "Dhuhr"; times.dhuhr.nameAr = "الظهر";
        times.dhuhr.time = QTime::fromString(query.value(5).toString(), "HH:mm");
        times.asr.name = "Asr"; times.asr.nameAr = "العصر";
        times.asr.time = QTime::fromString(query.value(6).toString(), "HH:mm");
        times.maghrib.name = "Maghrib"; times.maghrib.nameAr = "المغرب";
        times.maghrib.time = QTime::fromString(query.value(7).toString(), "HH:mm");
        times.isha.name = "Isha"; times.isha.nameAr = "العشاء";
        times.isha.time = QTime::fromString(query.value(8).toString(), "HH:mm");
        times.fajr.time = QTime::fromString(query.value(1).toString(), "HH:mm");
        times.sunrise.name = "Sunrise"; times.sunrise.nameAr = "الشروق";
        times.sunrise.time = QTime::fromString(query.value(2).toString(), "HH:mm");
        times.dhuhr.name = "Dhuhr"; times.dhuhr.nameAr = "الظهر";
        times.dhuhr.time = QTime::fromString(query.value(3).toString(), "HH:mm");
        times.asr.name = "Asr"; times.asr.nameAr = "العصر";
        times.asr.time = QTime::fromString(query.value(4).toString(), "HH:mm");
        times.maghrib.name = "Maghrib"; times.maghrib.nameAr = "المغرب";
        times.maghrib.time = QTime::fromString(query.value(5).toString(), "HH:mm");
        times.isha.name = "Isha"; times.isha.nameAr = "العشاء";
        times.isha.time = QTime::fromString(query.value(6).toString(), "HH:mm");
    }
    return times;
}

void StorageService::savePrayerTimes(const QString &city, const QString &country, int method,
                                     const DailyPrayerTimes &times)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO prayer_cache "
                  "(date, city, country, method, hijri_day, hijri_month_ar, hijri_year, "
                  "fajr, sunrise, dhuhr, asr, maghrib, isha, fetched_at) "
                  "VALUES (:date, :city, :country, :method, :hday, :hmonth, :hyear, "
                  ":fajr, :sunrise, :dhuhr, :asr, :maghrib, :isha, :now)");
    query.bindValue(":date", times.miladiDate.toString("yyyy-MM-dd"));
    query.bindValue(":city", city);
    query.bindValue(":country", country);
    query.bindValue(":method", method);
    query.bindValue(":hday", times.hijriDay);
    query.bindValue(":hmonth", times.hijriMonthAr);
    query.bindValue(":hyear", times.hijriYear);
    query.bindValue(":fajr", times.fajr.time.toString("HH:mm"));
    query.bindValue(":sunrise", times.sunrise.time.toString("HH:mm"));
    query.bindValue(":dhuhr", times.dhuhr.time.toString("HH:mm"));
    query.bindValue(":asr", times.asr.time.toString("HH:mm"));
    query.bindValue(":maghrib", times.maghrib.time.toString("HH:mm"));
    query.bindValue(":isha", times.isha.time.toString("HH:mm"));
    query.bindValue(":now", QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!query.exec())
        qWarning() << "Failed to save prayer times:" << query.lastError().text();
}

NidaSettings StorageService::loadSettings()
{
    NidaSettings s;
    QSqlQuery query(m_db);
    query.exec("SELECT key, value FROM settings");
    while (query.next()) {
        QString key = query.value(0).toString();
        QString val = query.value(1).toString();
        if (key == "city") s.city = val;
        else if (key == "country") s.country = val;
        else if (key == "method") s.method = val.toInt();
        else if (key == "selected_adhan") s.selectedAdhan = val;
        else if (key == "volume") s.volume = val.toInt();
        else if (key == "startup_enabled") s.startupEnabled = (val == "true");
        else if (key == "dark_theme") s.darkTheme = (val == "true");
        else if (key == "language") s.language = val;
    }
    return s;
}

void StorageService::saveSettings(const NidaSettings &settings)
{
    auto set = [&](const QString &key, const QString &value) {
        QSqlQuery q(m_db);
        q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:k, :v)");
        q.bindValue(":k", key);
        q.bindValue(":v", value);
        q.exec();
    };
    set("city", settings.city);
    set("country", settings.country);
    set("method", QString::number(settings.method));
    set("selected_adhan", settings.selectedAdhan);
    set("volume", QString::number(settings.volume));
    set("startup_enabled", settings.startupEnabled ? "true" : "false");
    set("dark_theme", settings.darkTheme ? "true" : "false");
    set("language", settings.language);
}

bool StorageService::isAdhanEnabled(const QString &prayerName)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT enabled FROM adhan_toggles WHERE prayer_name = :name");
    query.bindValue(":name", prayerName);
    if (query.exec() && query.next())
        return query.value(0).toInt() != 0;
    return true;
}

void StorageService::setAdhanEnabled(const QString &prayerName, bool enabled)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO adhan_toggles (prayer_name, enabled) VALUES (:name, :val)");
    query.bindValue(":name", prayerName);
    query.bindValue(":val", enabled ? 1 : 0);
    query.exec();
}
