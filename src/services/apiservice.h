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
    bool isOnline() const;

signals:
    void prayerTimesFetched(const DailyPrayerTimes &times);
    void fetchError(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    DailyPrayerTimes parseTimings(const QByteArray &data);
    QNetworkAccessManager *m_nam;
};

#endif
