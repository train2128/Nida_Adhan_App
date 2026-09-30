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
        if (e->time.isValid() && e->time > now)
            return e;
    }
    return fajr.time.isValid() ? &fajr : nullptr;
}

int DailyPrayerTimes::secondsUntilNext() const
{
    QDateTime now = QDateTime::currentDateTime();
    const PrayerTimeEntry* np = nextPrayer();
    if (!np || !np->time.isValid()) return 0;
    QDateTime nextTime(miladiDate.isValid() ? miladiDate : now.date(), np->time);
    if (nextTime <= now)
        nextTime = nextTime.addDays(1);
    return static_cast<int>(now.secsTo(nextTime));
}

bool DailyPrayerTimes::isValid() const
{
    return fajr.time.isValid() && sunrise.time.isValid()
        && dhuhr.time.isValid() && asr.time.isValid()
        && maghrib.time.isValid() && isha.time.isValid()
        && miladiDate.isValid();
}

QString formatHijriDate(const QString &day, const QString &monthAr, const QString &year)
{
    return day + " " + monthAr + " " + year;
}
