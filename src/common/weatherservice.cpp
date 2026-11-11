#include "weatherservice.h"

#include "settingscontroller.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include <cmath>

namespace
{

constexpr auto UserAgent = "StellarShell/0.1 (Qt6)";

QNetworkRequest makeRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(UserAgent));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

} // namespace

WeatherService::WeatherService(SettingsController *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_timer.setInterval(15 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &WeatherService::refresh);
    m_timer.start();

    // Give the shell time to paint before the first network request.
    QTimer::singleShot(1200, this, &WeatherService::reloadLocation);
}

void WeatherService::configure()
{
    m_autoLocate = true;
    if (m_settings) {
        m_settings->reloadSettings();
        m_autoLocate = m_settings->weatherAutoLocate();
        const QString configuredCity = m_settings->weatherCity();
        if (!configuredCity.isEmpty()) {
            m_city = configuredCity;
        }
    }

    // Las variables de entorno siguen teniendo prioridad para pruebas y
    // sesiones personalizadas.
    if (!qEnvironmentVariableIsEmpty("STELLAR_WEATHER_CITY")) {
        m_city = qEnvironmentVariable("STELLAR_WEATHER_CITY");
        m_autoLocate = false;
    }
    if (!qEnvironmentVariableIsEmpty("STELLAR_WEATHER_LAT")
        && !qEnvironmentVariableIsEmpty("STELLAR_WEATHER_LON")) {
        m_latitude = qEnvironmentVariable("STELLAR_WEATHER_LAT");
        m_longitude = qEnvironmentVariable("STELLAR_WEATHER_LON");
        m_locationResolved = true;
    }
}

void WeatherService::refresh()
{
    if (m_locationResolved) {
        requestWeather();
    } else {
        reloadLocation();
    }
}

void WeatherService::reloadLocation()
{
    if (m_resolving) {
        return;
    }
    configure();

    if (m_locationResolved) {
        requestWeather();
        return;
    }

    if (m_autoLocate) {
        requestIpLocation();
    } else if (!m_city.trimmed().isEmpty()) {
        requestCityGeocode();
    } else {
        useFallbackLocation();
    }
}

void WeatherService::useFallbackLocation()
{
    m_city = QStringLiteral("Madrid");
    m_latitude = QStringLiteral("40.4168");
    m_longitude = QStringLiteral("-3.7038");
    m_locationResolved = true;
    m_resolving = false;
    requestWeather();
}

void WeatherService::requestIpLocation()
{
    m_resolving = true;

    // ipwho.is es gratuito y devuelve JSON con ciudad y coordenadas. Se usa
    // ipapi.co como respaldo si el primero no responde.
    const QUrl url(QStringLiteral("https://ipwho.is/"));
    QNetworkReply *reply = m_network.get(makeRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        parseIpLocation(reply, true);
    });
}

void WeatherService::parseIpLocation(QNetworkReply *reply, bool fallback)
{
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        if (document.isObject()) {
            const QJsonObject root = document.object();
            const bool success = root.value(QStringLiteral("success")).toBool(true);
            const double latitude = root.value(QStringLiteral("latitude")).toDouble(999.0);
            const double longitude = root.value(QStringLiteral("longitude")).toDouble(999.0);
            const QString city = root.value(QStringLiteral("city")).toString().trimmed();
            if (success && latitude >= -90.0 && latitude <= 90.0
                && longitude >= -180.0 && longitude <= 180.0) {
                m_latitude = QString::number(latitude, 'f', 4);
                m_longitude = QString::number(longitude, 'f', 4);
                if (!city.isEmpty()) {
                    m_city = city;
                }
                m_locationResolved = true;
                m_resolving = false;
                emit weatherChanged();
                requestWeather();
                return;
            }
        }
    }

    if (fallback) {
        const QUrl url(QStringLiteral("https://ipapi.co/json/"));
        QNetworkReply *next = m_network.get(makeRequest(url));
        connect(next, &QNetworkReply::finished, this, [this, next] {
            next->deleteLater();
            parseIpLocation(next, false);
        });
        return;
    }

    useFallbackLocation();
}

void WeatherService::requestCityGeocode()
{
    m_resolving = true;

    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), m_city.trimmed());
    query.addQueryItem(QStringLiteral("count"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("language"), QStringLiteral("es"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(query);

    QNetworkReply *reply = m_network.get(makeRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        parseCityGeocode(reply);
    });
}

void WeatherService::parseCityGeocode(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray results = document.object().value(QStringLiteral("results")).toArray();
        if (!results.isEmpty()) {
            const QJsonObject place = results.first().toObject();
            const double latitude = place.value(QStringLiteral("latitude")).toDouble(999.0);
            const double longitude = place.value(QStringLiteral("longitude")).toDouble(999.0);
            if (latitude >= -90.0 && latitude <= 90.0
                && longitude >= -180.0 && longitude <= 180.0) {
                m_latitude = QString::number(latitude, 'f', 4);
                m_longitude = QString::number(longitude, 'f', 4);
                const QString name = place.value(QStringLiteral("name")).toString().trimmed();
                if (!name.isEmpty()) {
                    m_city = name;
                }
                m_locationResolved = true;
                m_resolving = false;
                emit weatherChanged();
                requestWeather();
                return;
            }
        }
    }

    useFallbackLocation();
}

void WeatherService::requestWeather()
{
    if (m_latitude.isEmpty() || m_longitude.isEmpty()) {
        useFallbackLocation();
        return;
    }

    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), m_latitude);
    query.addQueryItem(QStringLiteral("longitude"), m_longitude);
    query.addQueryItem(QStringLiteral("current_weather"), QStringLiteral("true"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    url.setQuery(query);

    QNetworkReply *reply = m_network.get(makeRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        parseWeather(reply);
    });
}

void WeatherService::parseWeather(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    const QByteArray payload = reply->readAll();
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    const QJsonObject current = root.value(QStringLiteral("current_weather")).toObject();
    if (current.isEmpty()) {
        return;
    }

    const int code = current.value(QStringLiteral("weathercode")).toInt();
    const double temperature = current.value(QStringLiteral("temperature")).toDouble();

    const bool wasAvailable = m_available;
    m_available = true;
    applyWeatherCode(code, temperature);

    if (!wasAvailable) {
        emit weatherChanged();
    }
}

void WeatherService::applyWeatherCode(int code, double temperatureCelsius)
{
    QString condition;
    QString icon;

    if (code == 0) {
        condition = tr("Despejado");
        icon = QStringLiteral("sun");
    } else if (code == 1 || code == 2) {
        condition = tr("Parcialmente nublado");
        icon = QStringLiteral("partly");
    } else if (code == 3) {
        condition = tr("Nublado");
        icon = QStringLiteral("cloud");
    } else if (code == 45 || code == 48) {
        condition = tr("Niebla");
        icon = QStringLiteral("fog");
    } else if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67)) {
        condition = tr("Lluvia");
        icon = QStringLiteral("rain");
    } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
        condition = tr("Nieve");
        icon = QStringLiteral("snow");
    } else if (code >= 80 && code <= 82) {
        condition = tr("Chubascos");
        icon = QStringLiteral("rain");
    } else if (code >= 95) {
        condition = tr("Tormenta");
        icon = QStringLiteral("storm");
    } else {
        condition = tr("Variable");
        icon = QStringLiteral("partly");
    }

    m_temperature = QStringLiteral("%1°").arg(qRound(temperatureCelsius));
    m_condition = condition;
    m_iconKind = icon;
    emit weatherChanged();
}
