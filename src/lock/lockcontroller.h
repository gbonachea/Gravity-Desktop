#pragma once

#include <QObject>
#include <QString>

/*!
 * Autenticación del bloqueo de pantalla mediante PAM.
 *
 * La comprobación se ejecuta en un hilo aparte para que la interfaz siga
 * respondiendo mientras PAM consulta la contraseña del usuario.
 */
class LockController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authenticating READ authenticating NOTIFY authenticatingChanged)
    Q_PROPERTY(QString userName READ userName CONSTANT)

public:
    explicit LockController(QObject *parent = nullptr);

    bool authenticating() const { return m_authenticating; }
    QString userName() const { return m_userName; }

    Q_INVOKABLE void authenticate(const QString &password);

signals:
    void authenticatingChanged();
    void authenticationFailed(const QString &message);
    void authenticated();

private:
    void finish(bool success, const QString &message);

    QString m_userName;
    bool m_authenticating = false;
};
