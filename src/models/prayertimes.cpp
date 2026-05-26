#include "prayertimes.h"
#include <QDateTime>
#include <QDebug>

QVector<PrayerTimeEntry*> DailyPrayerTimes::allPrayers()
{
    return {&fajr, &sunrise, &dhuhr, &asr, &maghrib, &isha};
}

const PrayerTimeEntry* DailyPrayerTimes::nextPrayer() const
{
    QTime now = QTime::currentTime();
    const PrayerTimeEntry* entries[] = {&fajr, &sunrise, &dhuhr, &asr, &maghrib, &isha};
    for (const auto* e : entries) {
        if (e->time > now)
            return e;
    }
    return &fajr;
}

int DailyPrayerTimes::secondsUntilNext() const
{
    QDateTime now = QDateTime::currentDateTime();
    const PrayerTimeEntry* np = nextPrayer();
    if (!np) return 0;
    QDateTime nextTime(miladiDate, np->time);
    if (nextTime <= now)
        nextTime = nextTime.addDays(1);
    return static_cast<int>(now.secsTo(nextTime));
}

QString formatHijriDate(const QString &day, const QString &monthAr, const QString &year)
{
    return day + " " + monthAr + " " + year;
}
