#include "apiservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkInformation>

ApiService::ApiService(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &ApiService::onReplyFinished);
}

bool ApiService::isOnline() const
{
    auto *info = QNetworkInformation::instance();
    if (info && info->reachability() == QNetworkInformation::Reachability::Online)
        return true;
    return false;
}

void ApiService::fetchPrayerTimes(const QString &city, const QString &country, int method)
{
    QDate today = QDate::currentDate();
    QString dateStr = today.toString("dd-MM-yyyy");
    QString url = QString(
        "https://api.aladhan.com/v1/timingsByCity?"
        "city=%1&country=%2&method=%3&date=%4"
    ).arg(city, country).arg(method).arg(dateStr);

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.0");
    m_nam->get(req);
}

void ApiService::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(reply->errorString());
        return;
    }
    DailyPrayerTimes times = parseTimings(reply->readAll());
    emit prayerTimesFetched(times);
}

DailyPrayerTimes ApiService::parseTimings(const QByteArray &data)
{
    DailyPrayerTimes times;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON response";
        return times;
    }

    QJsonObject root = doc.object();
    if (root.value("code").toInt() != 200)
        return times;

    QJsonObject dataObj = root.value("data").toObject();
    QJsonObject timings = dataObj.value("timings").toObject();
    QJsonObject dateObj = dataObj.value("date").toObject();
    QJsonObject hijriObj = dateObj.value("hijri").toObject();

    times.miladiDate = QDate::currentDate();
    times.hijriDay = hijriObj.value("day").toString();
    times.hijriMonthAr = hijriObj.value("month").toObject().value("ar").toString();
    times.hijriYear = hijriObj.value("year").toString();

    auto toTime = [](const QString &s) {
        return QTime::fromString(s.left(5), "HH:mm");
    };

    times.fajr.name = "Fajr"; times.fajr.nameAr = "الفجر";
    times.fajr.time = toTime(timings.value("Fajr").toString());

    times.sunrise.name = "Sunrise"; times.sunrise.nameAr = "الشروق";
    times.sunrise.time = toTime(timings.value("Sunrise").toString());

    times.dhuhr.name = "Dhuhr"; times.dhuhr.nameAr = "الظهر";
    times.dhuhr.time = toTime(timings.value("Dhuhr").toString());

    times.asr.name = "Asr"; times.asr.nameAr = "العصر";
    times.asr.time = toTime(timings.value("Asr").toString());

    times.maghrib.name = "Maghrib"; times.maghrib.nameAr = "المغرب";
    times.maghrib.time = toTime(timings.value("Maghrib").toString());

    times.isha.name = "Isha"; times.isha.nameAr = "العشاء";
    times.isha.time = toTime(timings.value("Isha").toString());

    qDebug() << "Parsed prayer times:" << times.fajr.time << times.dhuhr.time << times.maghrib.time;
    return times;
}
