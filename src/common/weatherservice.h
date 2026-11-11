#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>

class QNetworkReply;
class SettingsController;

/*!
 * Cliente de Open-Meteo. La ubicación puede fijarse con una ciudad, obtenerse
 * automáticamente por red (geolocalización IP) o mediante las variables de
 * entorno STELLAR_WEATHER_CITY / STELLAR_WEATHER_LAT / STELLAR_WEATHER_LON.
 */
class WeatherService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString temperature READ temperature NOTIFY weatherChanged)
    Q_PROPERTY(QString condition READ condition NOTIFY weatherChanged)
    Q_PROPERTY(QString iconKind READ iconKind NOTIFY weatherChanged)
    Q_PROPERTY(QString city READ city NOTIFY weatherChanged)
    Q_PROPERTY(bool available READ available NOTIFY weatherChanged)
    Q_PROPERTY(bool autoLocate READ autoLocate NOTIFY weatherChanged)

public:
    explicit WeatherService(SettingsController *settings = nullptr,
                            QObject *parent = nullptr);

    QString temperature() const { return m_temperature; }
    QString condition() const { return m_condition; }
    QString iconKind() const { return m_iconKind; }
    QString city() const { return m_city; }
    bool available() const { return m_available; }
    bool autoLocate() const { return m_autoLocate; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void reloadLocation();

signals:
    void weatherChanged();

private:
    void configure();
    void requestIpLocation();
    void requestCityGeocode();
    void requestWeather();
    void parseIpLocation(QNetworkReply *reply, bool fallback);
    void parseCityGeocode(QNetworkReply *reply);
    void parseWeather(QNetworkReply *reply);
    void applyWeatherCode(int code, double temperatureCelsius);
    void useFallbackLocation();

    SettingsController *m_settings = nullptr;
    QNetworkAccessManager m_network;
    QTimer m_timer;
    QString m_temperature = QStringLiteral("22°");
    QString m_condition = QStringLiteral("Parcialmente nublado");
    QString m_iconKind = QStringLiteral("partly");
    QString m_city = QStringLiteral("Madrid");
    QString m_latitude = QStringLiteral("40.4168");
    QString m_longitude = QStringLiteral("-3.7038");
    bool m_autoLocate = true;
    bool m_available = false;
    bool m_locationResolved = false;
    bool m_resolving = false;
};
