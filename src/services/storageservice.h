#ifndef STORAGESERVICE_H
#define STORAGESERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include "models/prayertimes.h"

class StorageService : public QObject
{
    Q_OBJECT
public:
    explicit StorageService(QObject *parent = nullptr);
    ~StorageService();

    bool initialize();

    // Prayer times cache (legacy city/country key)
    bool hasValidCache(const QString &city, const QString &country, int method);
    DailyPrayerTimes loadPrayerTimes(const QString &city, const QString &country, int method);
    void savePrayerTimes(const QString &city, const QString &country, int method,
                         const DailyPrayerTimes &times);

    // Prayer times cache (precise coords key, rounded to ~1km for stability)
    bool hasValidCacheForCoords(double lat, double lon, int method);
    DailyPrayerTimes loadPrayerTimesForCoords(double lat, double lon, int method);
    void savePrayerTimesForCoords(double lat, double lon, int method,
                                  const DailyPrayerTimes &times);

    // Settings
    NidaSettings loadSettings();
    void saveSettings(const NidaSettings &settings);

    // Per-prayer adhan toggle
    bool isAdhanEnabled(const QString &prayerName);
    void setAdhanEnabled(const QString &prayerName, bool enabled);

private:
    void createTables();
    QSqlDatabase m_db;
};

#endif
