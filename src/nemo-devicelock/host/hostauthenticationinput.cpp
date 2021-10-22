/*
 * Copyright (C) 2016 Jolla Ltd
 * Contact: Andrew den Exter <andrew.den.exter@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "hostauthenticationinput.h"

#include "settingswatcher.h"

#include <QFile>
#include <sailfish-minui/alphanumtool.h>

namespace NemoDeviceLock
{

static const auto clientInterface = QStringLiteral("org.nemomobile.devicelock.client.AuthenticationInput");

HostAuthenticationInputAdaptor::HostAuthenticationInputAdaptor(
        HostAuthenticationInput *authenticationInput)
    : QDBusAbstractAdaptor(authenticationInput)
    , m_authenticationInput(authenticationInput)
{
}

void HostAuthenticationInputAdaptor::SetRegistered(const QDBusObjectPath &path, bool registered)
{
    m_authenticationInput->setRegistered(path.path(), registered);
}

void HostAuthenticationInputAdaptor::SetActive(const QDBusObjectPath &path, bool active)
{
    m_authenticationInput->setActive(path.path(), active);
}

void HostAuthenticationInputAdaptor::EnterSecurityCode(const QDBusObjectPath &path, const QString &code)
{
    m_authenticationInput->handleEnterSecurityCode(path.path(), code);
}

void HostAuthenticationInputAdaptor::RequestSecurityCode(const QDBusObjectPath &path)
{
    m_authenticationInput->handleRequestSecurityCode(path.path());
}

void HostAuthenticationInputAdaptor::Cancel(const QDBusObjectPath &path)
{
     m_authenticationInput->handleCancel(path.path());
}

void HostAuthenticationInputAdaptor::Authorize(const QDBusObjectPath &path)
{
     m_authenticationInput->handleAuthorize(path.path());
}

HostAuthenticationInput::HostAuthenticationInput(
        const QString &path, Authenticator::Methods supportedMethods, QObject *parent)
    : HostObject(path, parent)
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_supportedMethods(supportedMethods | Authenticator::Confirmation) // Basic yes/no confirmation is always supported.
    , m_activeMethods()
    , m_authenticating(false)
{
}

HostAuthenticationInput::~HostAuthenticationInput()
{
}

void HostAuthenticationInput::authorize()
{
}

void HostAuthenticationInput::authenticationStarted(
        Authenticator::Methods methods,
        uint authenticatingPid,
        AuthenticationInput::Feedback)
{
    Q_UNUSED(authenticatingPid);

    qCDebug(daemon, "Authentication started");

    m_authenticating = true;
    m_activeMethods = methods & m_supportedMethods;
}

void HostAuthenticationInput::startAuthentication(
        AuthenticationInput::Feedback feedback,
        const QVariantMap &data,
        Authenticator::Methods methods)
{
    const uint pid = connectionPid(QDBusContext::connection());
    if (pid != 0) {
        startAuthentication(feedback, pid, data, methods);
    }
}

void HostAuthenticationInput::startAuthentication(
        AuthenticationInput::Feedback feedback,
        uint authenticatingPid,
        const QVariantMap &data,
        Authenticator::Methods methods)
{
    qCDebug(daemon, "Authentication started");

    if (!m_inputStack.isEmpty()) {
        authenticationStarted(methods, authenticatingPid, feedback);

        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("AuthenticationStarted"),
                    authenticatingPid,
                    uint(m_activeMethods),
                    uint(feedback),
                    data);
    }
}

void HostAuthenticationInput::authenticationUnavailable(AuthenticationInput::Error error)
{
    const uint pid = connectionPid(QDBusContext::connection());
    if (pid != 0) {
        authenticationUnavailable(error, pid);
    }
}

void HostAuthenticationInput::authenticationUnavailable(
        AuthenticationInput::Error error, uint authenticatingPid)
{
    qCDebug(daemon, "Authentication unavailable");

    if (!m_inputStack.isEmpty()) {
        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("AuthenticationUnavailable"),
                    authenticatingPid,
                    uint(error));
    }
}

void HostAuthenticationInput::authenticationResumed(
        AuthenticationInput::Feedback feedback,
        const QVariantMap &data,
        Authenticator::Methods utilizedMethods)
{
    qCDebug(daemon, "Authentication resumed");

    if (!m_inputStack.isEmpty()) {
        if (utilizedMethods != 0) { // Utilized methods can be empty if there is no change.
            utilizedMethods = m_activeMethods;
        }
        m_activeMethods = utilizedMethods & m_supportedMethods;

        m_authenticating = true;
        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("AuthenticationResumed"),
                    uint(m_activeMethods),
                    uint(feedback),
                    data);
    }
}

void HostAuthenticationInput::authenticationEvaluating()
{
    if (m_authenticating && !m_inputStack.isEmpty()) {
        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("AuthenticationEvaluating"));
    }
}

void HostAuthenticationInput::authenticationProgress(int current, int maximum)
{
    if (m_authenticating && !m_inputStack.isEmpty()) {
        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("AuthenticationProgress"),
                    current,
                    maximum);
    }
}

void HostAuthenticationInput::authenticationEnded(bool confirmed)
{
    if (m_authenticating) {
        qCDebug(daemon, "Authentication ended: %s", confirmed ? "true" : "false");

        m_authenticating = false;
        authenticationInactive();

        if (!m_inputStack.isEmpty()) {
            NemoDBus::send(
                        m_inputStack.last().connection,
                        m_inputStack.last().path,
                        clientInterface,
                        QStringLiteral("AuthenticationEnded"),
                        confirmed);
        }
    }
}

void HostAuthenticationInput::setRegistered(const QString &path, bool registered)
{
    const auto pid = connectionPid(QDBusContext::connection());

    if (pid == 0 || !authorizeInput(pid)) {
        QDBusContext::sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    const auto connection = QDBusContext::connection().name();

    if (registered) {
        for (int i = 0; i < m_inputStack.count(); ++i) {
            const auto &input = m_inputStack.at(i);
            if (input.connection != connection || input.path != path) {
                continue;
            } else if (i == m_inputStack.count() - 1) {
                return;
            } else {
                m_inputStack.removeAt(i);
                break;
            }
        }

        if (!m_inputStack.isEmpty()) {
            cancel();
        }

        m_inputStack.append(Input(connection, path));
    } else for (int i = 0; i < m_inputStack.count(); ++i) {
        const auto &input = m_inputStack.at(i);

        if (input.connection != connection || input.path != path) {
            continue;
        }

        if (i == m_inputStack.count() - 1) {
            cancel();
        }

        m_inputStack.removeAt(i);

        break;
    }
}

void HostAuthenticationInput::setActive(const QString &path, bool active)
{
    const auto connection = QDBusContext::connection().name();

    if (m_authenticating
            && !m_inputStack.isEmpty()
            && m_inputStack.last().connection == connection
            && m_inputStack.last().path == path) {
        if (active) {
            authenticationActive(m_activeMethods);
        } else {
            authenticationInactive();
        }
    }
}

void HostAuthenticationInput::authenticationActive(Authenticator::Methods)
{
}

void HostAuthenticationInput::authenticationInactive()
{
}

bool HostAuthenticationInput::authorizeInput(unsigned long)
{
    return true;
}

int HostAuthenticationInput::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

int HostAuthenticationInput::currentAttempts() const
{
    return m_settings->currentAttempts;
}

AuthenticationInput::CodeGeneration HostAuthenticationInput::codeGeneration() const
{
    return m_settings->codeGeneration;
}

QString HostAuthenticationInput::generateCode() const
{
    quint64 number = 0;
    QFile file(QStringLiteral("/dev/random"));
    if (file.open(QIODevice::ReadOnly)) {
        file.read(reinterpret_cast<char *>(&number), sizeof(number));
        file.close();
    }
    return QString::number(number).right(m_settings->minimumLength).rightJustified(m_settings->minimumLength, QLatin1Char('0'));
}

void HostAuthenticationInput::feedback(
        AuthenticationInput::Feedback feedback,
        const QVariantMap &data,
        Authenticator::Methods utilizedMethods)
{
    if (!m_inputStack.isEmpty()) {
        if (utilizedMethods != 0) { // Utilized methods can be empty if there is no change.
            utilizedMethods = m_activeMethods;
        }
        m_activeMethods = utilizedMethods & m_supportedMethods;

        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("Feedback"),
                    uint(feedback),
                    data,
                    uint(m_activeMethods));
    }
}

void HostAuthenticationInput::feedback(
        AuthenticationInput::Feedback feedback,
        int attemptsRemaining,
        Authenticator::Methods utilizedMethods)
{
    QVariantMap data;
    data.insert(QStringLiteral("attemptsRemaining"), attemptsRemaining);
    HostAuthenticationInput::feedback(feedback, data, utilizedMethods);
}

void HostAuthenticationInput::lockedOut()
{
    QVariantMap data;
    lockedOut(availability(&data), &HostAuthenticationInput::abortAuthentication, data);
}

void HostAuthenticationInput::lockedOut(
        Availability availability,
        void (HostAuthenticationInput::*errorFunction)(AuthenticationInput::Error error),
        const QVariantMap &data)
{
    switch (availability) {
    case CodeEntryLockedRecoverable:
        (this->*errorFunction)(AuthenticationInput::MaximumAttemptsExceeded);
        feedback(AuthenticationInput::TemporarilyLocked, data);
        break;
    case CodeEntryLockedPermanent:
        (this->*errorFunction)(AuthenticationInput::MaximumAttemptsExceeded);
        feedback(AuthenticationInput::PermanentlyLocked, data);
        break;
    case ManagerLockedRecoverable:
        (this->*errorFunction)(AuthenticationInput::LockedByManager);
        feedback(AuthenticationInput::ContactSupport, data);
        break;
    case ManagerLockedPermanent:
        (this->*errorFunction)(AuthenticationInput::LockedByManager);
        feedback(AuthenticationInput::PermanentlyLocked, data);
        break;
    default:
        // Locked out but availability doesn't reflect this.  This shouldn't be reachable
        // under normal circumstances.
        (this->*errorFunction)(AuthenticationInput::SoftwareError);
    }
}

bool HostAuthenticationInput::checkEncryptionCodeValidity(const QString &code)
{
    return Sailfish::MinUi::checkCodeValidity(code.toLocal8Bit().data());
}

void HostAuthenticationInput::abortAuthentication(AuthenticationInput::Error error)
{
    if (m_authenticating) {
        m_authenticating = false;
        authenticationInactive();
    }
    if (!m_inputStack.isEmpty()) {
        NemoDBus::send(
                    m_inputStack.last().connection,
                    m_inputStack.last().path,
                    clientInterface,
                    QStringLiteral("Error"),
                    uint(error));
    }
}

void HostAuthenticationInput::clientDisconnected(const QString &connection)
{
    for (int i = 0; i < m_inputStack.count(); ) {
        if (m_inputStack.at(i).connection == connection) {

            if (i == m_inputStack.count() - 1) {
                cancel();
            }

            m_inputStack.removeAt(i);
        } else {
            ++i;
        }
    }

    HostObject::clientDisconnected(connection);
}

void HostAuthenticationInput::handleEnterSecurityCode(const QString &path, const QString &code)
{
    const auto connection = QDBusContext::connection().name();
    if (!m_inputStack.isEmpty()
            && m_inputStack.last().connection == connection
            && m_inputStack.last().path == path) {
        enterSecurityCode(code);
    }
}

void HostAuthenticationInput::handleRequestSecurityCode(const QString &path)
{
    const auto connection = QDBusContext::connection().name();
    if (!m_inputStack.isEmpty()
            && m_inputStack.last().connection == connection
            && m_inputStack.last().path == path) {
        requestSecurityCode();
    }
}

void HostAuthenticationInput::handleCancel(const QString &path)
{
    const auto connection = QDBusContext::connection().name();
    if (!m_inputStack.isEmpty()
            && m_inputStack.last().connection == connection
            && m_inputStack.last().path == path) {
        cancel();
    }
}

void HostAuthenticationInput::handleAuthorize(const QString &path)
{
    const auto connection = QDBusContext::connection().name();
    if (!m_inputStack.isEmpty()
            && m_inputStack.last().connection == connection
            && m_inputStack.last().path == path) {
        authorize();
    }
}

}
