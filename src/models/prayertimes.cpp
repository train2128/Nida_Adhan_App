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

QList<QPair<int, QString>> calculationMethods()
{
    return {
        {3, QStringLiteral("Muslim World League")},
        {2, QStringLiteral("Islamic Society of North America")},
        {5, QStringLiteral("Egyptian General Authority")},
        {4, QStringLiteral("Umm Al-Qura (Makkah)")},
        {1, QStringLiteral("University of Islamic Sciences (Karachi)")},
        {7, QStringLiteral("Institute of Geophysics (Tehran)")},
        {8, QStringLiteral("Gulf Region")},
        {9, QStringLiteral("Kuwait")},
        {10, QStringLiteral("Qatar")},
        {11, QStringLiteral("Majlis Ugama (Singapore)")},
        {12, QStringLiteral("Union Organization (France)")},
        {13, QStringLiteral("Diyanet (Turkey)")},
        {14, QStringLiteral("Spiritual Administration (Russia)")},
        {15, QStringLiteral("Moonsighting Committee")},
        {16, QStringLiteral("Dubai")},
        {17, QStringLiteral("JAKIM (Malaysia)")},
        {18, QStringLiteral("Tunisia")},
        {19, QStringLiteral("Algeria")},
        {20, QStringLiteral("Kemenag (Indonesia)")},
        {21, QStringLiteral("Morocco")},
        {22, QStringLiteral("Comunidade de Lisboa (Portugal)")},
        {23, QStringLiteral("Ministry of Awqaf (Jordan)")},
    };
}

QString calculationMethodName(int method)
{
    for (const auto &m : calculationMethods()) {
        if (m.first == method)
            return m.second;
    }
    return QStringLiteral("Method %1").arg(method);
}

QString formatHijriDate(const QString &day, const QString &monthAr, const QString &year)
{
    return day + " " + monthAr + " " + year;
}
