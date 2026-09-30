#include "locationservice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QtGlobal>
#include <QDebug>

namespace {
QString pick(const QJsonObject &props, const char *const *keys, int n)
{
    for (int i = 0; i < n; ++i) {
        const QString v = props.value(QLatin1String(keys[i])).toString().trimmed();
        if (!v.isEmpty())
            return v;
    }
    return {};
}
}

LocationService::LocationService(QObject *parent)
    : QObject(parent)
    , m_searchNam(new QNetworkAccessManager(this))
    , m_detectNam(new QNetworkAccessManager(this))
{
    connect(m_searchNam, &QNetworkAccessManager::finished,
            this, &LocationService::onSearchReply);
    connect(m_detectNam, &QNetworkAccessManager::finished,
            this, &LocationService::onDetectReply);
}

void LocationService::searchLocations(const QString &query, const QString &lang)
{
    const QString q = query.trimmed();
    if (q.length() < 2)
        return;
    QUrl url("https://photon.komoot.io/api");
    QUrlQuery params;
    params.addQueryItem("q", q);
    params.addQueryItem("limit", QStringLiteral("6"));
    params.addQueryItem("lang", lang == QLatin1String("ar") ? QStringLiteral("ar") : QStringLiteral("en"));
    url.setQuery(params);

    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_searchNam->get(req);
    reply->setProperty("searchSeq", ++m_searchSeq);
}

void LocationService::detectCurrentLocation()
{
    QNetworkRequest req{QUrl(QStringLiteral("https://ipapi.co/json/"))};
    req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
    QNetworkReply *reply = m_detectNam->get(req);
    reply->setProperty("detectSource", QStringLiteral("ipapi-co"));
}

void LocationService::onSearchReply(QNetworkReply *reply)
{
    reply->deleteLater();
    const int seq = reply->property("searchSeq").toInt();
    if (seq <= m_lastHandledSeq)
        return; // stale response from an older keystroke
    if (reply->error() != QNetworkReply::NoError) {
        // Don't spam errors for superseded requests.
        if (seq == m_searchSeq)
            emit searchError(reply->errorString());
        return;
    }
    m_lastHandledSeq = seq;

    QList<LocationResult> out;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
        return;
    const QJsonArray features = doc.object().value("features").toArray();
    for (const QJsonValue &f : features) {
        if (!f.isObject())
            continue;
        LocationResult r = fromPhotonFeature(f.toObject());
        if (r.valid)
            out.append(r);
    }
    emit searchFinished(out);
}

void LocationService::onDetectReply(QNetworkReply *reply)
{
    reply->deleteLater();
    const QString source = reply->property("detectSource").toString();
    if (reply->error() != QNetworkReply::NoError) {
        if (source == QLatin1String("ipapi-co")) {
            // Fallback: no-key HTTP endpoint with the same city/country/lat/lon.
            QNetworkRequest req{QUrl(QStringLiteral("http://ip-api.com/json/"))};
            req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
            QNetworkReply *fb = m_detectNam->get(req);
            fb->setProperty("detectSource", QStringLiteral("ip-api-com"));
            return;
        }
        emit detectError(reply->errorString());
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        if (source == QLatin1String("ipapi-co")) {
            QNetworkRequest req{QUrl(QStringLiteral("http://ip-api.com/json/"))};
            req.setHeader(QNetworkRequest::UserAgentHeader, "Nida/1.1");
            QNetworkReply *fb = m_detectNam->get(req);
            fb->setProperty("detectSource", QStringLiteral("ip-api-com"));
            return;
        }
        emit detectError(QStringLiteral("Invalid location response"));
        return;
    }
    LocationResult r = source == QLatin1String("ip-api-com")
        ? fromIpApiCom(doc.object())
        : fromIpApi(doc.object());
    if (!r.valid) {
        emit detectError(QStringLiteral("Could not determine location"));
        return;
    }
    emit detectFinished(r);
}

LocationResult LocationService::fromPhotonFeature(const QJsonObject &feature)
{
    LocationResult r;
    const QJsonObject props = feature.value("properties").toObject();
    const QJsonObject geom = feature.value("geometry").toObject();
    const QJsonArray coords = geom.value("coordinates").toArray();
    if (coords.size() < 2)
        return r;

    static const char *cityKeys[] = {"city", "town", "village", "hamlet", "locality", "name"};
    static const char *stateKeys[] = {"state", "county", "district"};
    const QString name = props.value("name").toString().trimmed();
    const QString city = pick(props, cityKeys, 6);
    const QString state = pick(props, stateKeys, 3);
    const QString country = props.value("country").toString().trimmed();
    if (country.isEmpty())
        return r;

    // Prefer "City, Country"; fall back to "Name, State, Country" so suburbs
    // and villages still get a usable, exact label.
    const QString place = !city.isEmpty() ? city : name;
    if (place.isEmpty())
        return r;
    r.city = place;
    r.country = country;
    if (place == country)
        r.label = place;
    else if (!state.isEmpty() && state != place && state != country)
        r.label = QString("%1, %2, %3").arg(place, state, country);
    else
        r.label = QString("%1, %2").arg(place, country);

    r.longitude = coords.at(0).toDouble();
    r.latitude = coords.at(1).toDouble();
    r.valid = r.latitude >= -90.0 && r.latitude <= 90.0
        && r.longitude >= -180.0 && r.longitude <= 180.0
        && (r.latitude != 0.0 || r.longitude != 0.0);
    return r;
}

LocationResult LocationService::fromIpApi(const QJsonObject &obj)
{
    LocationResult r;
    if (obj.value("error").toBool(false))
        return r;
    r.city = obj.value("city").toString().trimmed();
    r.country = obj.value("country_name").toString().trimmed();
    r.latitude = obj.value("latitude").toDouble(qQNaN());
    r.longitude = obj.value("longitude").toDouble(qQNaN());
    if (r.city.isEmpty() || r.country.isEmpty())
        return r;
    if (qIsNaN(r.latitude) || qIsNaN(r.longitude))
        return r;
    r.label = QString("%1, %2").arg(r.city, r.country);
    r.valid = true;
    return r;
}

LocationResult LocationService::fromIpApiCom(const QJsonObject &obj)
{
    // http://ip-api.com/json/ -> {status, city, country, lat, lon}
    LocationResult r;
    if (obj.value("status").toString() != QLatin1String("success"))
        return r;
    r.city = obj.value("city").toString().trimmed();
    r.country = obj.value("country").toString().trimmed();
    r.latitude = obj.value("lat").toDouble(qQNaN());
    r.longitude = obj.value("lon").toDouble(qQNaN());
    if (r.city.isEmpty() || r.country.isEmpty())
        return r;
    if (qIsNaN(r.latitude) || qIsNaN(r.longitude))
        return r;
    r.label = QString("%1, %2").arg(r.city, r.country);
    r.valid = true;
    return r;
}
