#include "apiservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkInformation>
#include <QUrl>
#include <QUrlQuery>

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
    if (!info)
        return true; // No backend loaded: be optimistic, let the request fail gracefully.
    return info->reachability() == QNetworkInformation::Reachability::Online;
}

void ApiService::fetchPrayerTimes(const QString &city, const QString &country, int method)
{
    QUrl url("https://api.aladhan.com/v1/timingsByCity");
    QUrlQuery q;
    q.addQueryItem("city", city.trimmed());
    q.addQueryItem("country", country.trimmed());
    q.addQueryItem("method", QString::number(method));
    q.addQueryItem("date", QDate::currentDate().toString("dd-MM-yyyy"));
    url.setQuery(q);

    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
    m_nam->get(req);
}

void ApiService::fetchPrayerTimesByCoords(double latitude, double longitude, int method)
{
    QUrl url(QString("https://api.aladhan.com/v1/timings/%1")
                 .arg(QDate::currentDate().toString("dd-MM-yyyy")));
    QUrlQuery q;
    q.addQueryItem("latitude", QString::number(latitude, 'f', 6));
    q.addQueryItem("longitude", QString::number(longitude, 'f', 6));
    q.addQueryItem("method", QString::number(method));
    url.setQuery(q);

    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
    m_nam->get(req);
}

void ApiService::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(reply->errorString());
        return;
    }
    DailyPrayerTimes times;
    if (!parseTimings(reply->readAll(), &times)) {
        emit fetchError(QStringLiteral("Invalid prayer-times response"));
        return;
    }
    emit prayerTimesFetched(times);
}

bool ApiService::parseTimings(const QByteArray &data, DailyPrayerTimes *out)
{
    if (!out)
        return false;
    DailyPrayerTimes times;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON response";
        return false;
    }

    QJsonObject root = doc.object();
    if (root.value("code").toInt() != 200)
        return false;

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

    if (!times.isValid()) {
        qWarning() << "Parsed prayer times are incomplete";
        return false;
    }

    qDebug() << "Parsed prayer times:" << times.fajr.time << times.dhuhr.time << times.maghrib.time;
    *out = times;
    return true;
}
