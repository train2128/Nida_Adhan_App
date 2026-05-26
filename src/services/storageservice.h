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

    // Prayer times cache
    bool hasValidCache(const QString &city, const QString &country, int method);
    DailyPrayerTimes loadPrayerTimes(const QString &city, const QString &country, int method);
    void savePrayerTimes(const QString &city, const QString &country, int method,
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
