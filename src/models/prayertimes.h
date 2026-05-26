#ifndef PRAYERTIMES_H
#define PRAYERTIMES_H

#include <QString>
#include <QTime>
#include <QDate>
#include <QVector>

struct PrayerTimeEntry {
    QString name;
    QString nameAr;
    QTime time;
    bool adhanEnabled = true;
};

struct DailyPrayerTimes {
    QDate miladiDate;
    QString hijriDay;
    QString hijriMonthAr;
    QString hijriYear;
    PrayerTimeEntry fajr;
    PrayerTimeEntry sunrise;
    PrayerTimeEntry dhuhr;
    PrayerTimeEntry asr;
    PrayerTimeEntry maghrib;
    PrayerTimeEntry isha;

    QVector<PrayerTimeEntry*> allPrayers();
    const PrayerTimeEntry* nextPrayer() const;
    int secondsUntilNext() const;
};

struct NidaSettings {
    QString city = "Mecca";
    QString country = "Saudi Arabia";
    int method = 3;
    QString selectedAdhan = "Adhan-Makkah";
    int volume = 80;
    bool startupEnabled = true;
    bool darkTheme = true;
    QString language = "en";
};

QString formatHijriDate(const QString &day, const QString &monthAr, const QString &year);

enum class CalculationMethod {
    Jafari = 0,
    Karachi = 1,
    ISNA = 2,
    MWL = 3,
    Makkah = 4,
    Egypt = 5,
    Tehran = 7,
    Gulf = 8,
    Kuwait = 9,
    Qatar = 10,
    Singapore = 11,
    France = 12,
    Turkey = 13,
    Russia = 14,
    Moonsighting = 15,
    Dubai = 16,
    Jakarta = 17,
    Tunisia = 18,
    Algeria = 19,
    Kemenag = 20,
    Morocco = 21,
    Portugal = 22,
    Jordan = 23
};

#endif
