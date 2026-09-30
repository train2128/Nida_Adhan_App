#ifndef LOCATIONSERVICE_H
#define LOCATIONSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>

struct LocationResult {
    QString label;   // "Sousse, Tunisia"
    QString city;
    QString country;
    double latitude = 0.0;
    double longitude = 0.0;
    bool valid = false;
};
Q_DECLARE_METATYPE(LocationResult)

// Photon (OSM-based typeahead) for city search + IP-based "find my location".
// Both need internet; results are persisted to SQLite by the caller so the app
// keeps working offline afterwards.
class LocationService : public QObject
{
    Q_OBJECT
public:
    explicit LocationService(QObject *parent = nullptr);

    void searchLocations(const QString &query, const QString &lang = QStringLiteral("en"));
    void detectCurrentLocation();

signals:
    void searchFinished(const QList<LocationResult> &results);
    void searchError(const QString &error);
    void detectFinished(const LocationResult &result);
    void detectError(const QString &error);

private slots:
    void onSearchReply(QNetworkReply *reply);
    void onDetectReply(QNetworkReply *reply);

private:
    static LocationResult fromPhotonFeature(const QJsonObject &feature);
    static LocationResult fromIpApi(const QJsonObject &obj);
    static LocationResult fromIpApiCom(const QJsonObject &obj);

    QNetworkAccessManager *m_searchNam;
    QNetworkAccessManager *m_detectNam;
    int m_searchSeq = 0;
    int m_lastHandledSeq = 0;
};

#endif
