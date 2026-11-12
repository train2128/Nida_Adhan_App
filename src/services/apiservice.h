#ifndef APISERVICE_H
#define APISERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "models/prayertimes.h"

class ApiService : public QObject
{
    Q_OBJECT
public:
    explicit ApiService(QObject *parent = nullptr);

    void fetchPrayerTimes(const QString &city, const QString &country, int method);
    void fetchPrayerTimesByCoords(double latitude, double longitude, int method);
    bool isOnline() const;

signals:
    void prayerTimesFetched(const DailyPrayerTimes &times);
    void fetchError(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    bool parseTimings(const QByteArray &data, DailyPrayerTimes *out);
    void sendRequest(const QUrl &url);
    QNetworkAccessManager *m_nam;
    // Only the newest request's answer is used: an older in-flight reply
    // (e.g. for the previous city) must not be saved under new settings.
    int m_requestSeq = 0;
};

#endif
