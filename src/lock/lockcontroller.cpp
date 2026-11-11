#include "lockcontroller.h"

#include <QDir>
#include <QThread>

#include <security/pam_appl.h>

#include <cstdlib>
#include <cstring>

namespace
{

struct PamConversationData {
    QByteArray password;
};

int pamConversation(int numMessages, const struct pam_message **messages,
                    struct pam_response **responses, void *data)
{
    if (!data || !responses || numMessages <= 0) {
        return PAM_CONV_ERR;
    }

    auto *conversation = static_cast<PamConversationData *>(data);
    auto *replies = static_cast<struct pam_response *>(
        calloc(size_t(numMessages), sizeof(struct pam_response)));
    if (!replies) {
        return PAM_BUF_ERR;
    }

    for (int i = 0; i < numMessages; ++i) {
        switch (messages[i]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
        case PAM_PROMPT_ECHO_ON:
            replies[i].resp = strdup(conversation->password.constData());
            if (!replies[i].resp) {
                for (int j = 0; j < i; ++j) {
                    free(replies[j].resp);
                }
                free(replies);
                return PAM_BUF_ERR;
            }
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            break;
        default:
            break;
        }
    }

    *responses = replies;
    return PAM_SUCCESS;
}

bool pamAuthenticate(const QString &userName, const QString &password, QString *error)
{
    PamConversationData data;
    data.password = password.toUtf8();

    struct pam_conv conversation { pamConversation, &data };
    const QByteArray user = userName.toUtf8();
    pam_handle_t *handle = nullptr;

    int result = pam_start("login", user.constData(), &conversation, &handle);
    if (result != PAM_SUCCESS) {
        if (error) {
            *error = handle ? QString::fromUtf8(pam_strerror(handle, result))
                            : LockController::tr("No se pudo iniciar PAM");
        }
        if (handle) {
            pam_end(handle, result);
        }
        return false;
    }

    result = pam_authenticate(handle, 0);
    if (result == PAM_SUCCESS) {
        const int account = pam_acct_mgmt(handle, 0);
        // Una contraseña caducada sigue siendo una identidad válida: se deja
        // entrar al escritorio y el usuario podrá cambiarla desde el sistema.
        if (account != PAM_SUCCESS && account != PAM_NEW_AUTHTOK_REQD) {
            result = account;
        }
    }

    if (result != PAM_SUCCESS && error) {
        *error = QString::fromUtf8(pam_strerror(handle, result));
    }

    pam_end(handle, result);
    return result == PAM_SUCCESS;
}

QString sessionUserName()
{
    QString name = qEnvironmentVariable("USER");
    if (name.isEmpty()) {
        name = qEnvironmentVariable("LOGNAME");
    }
    if (name.isEmpty()) {
        name = qEnvironmentVariable("XDG_SESSION_USER");
    }
    if (name.isEmpty()) {
        name = QDir::home().dirName();
    }
    return name;
}

} // namespace

LockController::LockController(QObject *parent)
    : QObject(parent)
    , m_userName(sessionUserName())
{
}

void LockController::authenticate(const QString &password)
{
    if (m_authenticating) {
        return;
    }
    if (m_userName.isEmpty()) {
        emit authenticationFailed(tr("No se pudo determinar el usuario de la sesión"));
        return;
    }
    if (password.isEmpty()) {
        emit authenticationFailed(tr("Escribe tu contraseña"));
        return;
    }

    m_authenticating = true;
    emit authenticatingChanged();

    const QString user = m_userName;
    QThread *thread = QThread::create([this, user, password] {
        QString error;
        const bool ok = pamAuthenticate(user, password, &error);
        QMetaObject::invokeMethod(this, [this, ok, error] {
            finish(ok, error);
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void LockController::finish(bool success, const QString &message)
{
    m_authenticating = false;
    emit authenticatingChanged();

    if (success) {
        emit authenticated();
    } else {
        emit authenticationFailed(message.isEmpty()
                                      ? tr("Contraseña incorrecta")
                                      : message);
    }
}
