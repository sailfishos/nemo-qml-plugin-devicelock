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

#include "cliauthenticator.h"

#include "dbusutilities.h"
#include "lockcodewatcher.h"

#include <QDBusMessage>

#include <QDebug>

namespace NemoDeviceLock
{

CliAuthenticator::CliAuthenticator(QObject *parent)
    : HostAuthenticator(parent)
    , m_watcher(LockCodeWatcher::instance())
    , m_attemptCount(QStringLiteral("/desktop/nemo/devicelock/attempt_count"))
{
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &CliAuthenticator::availableMethodsChanged);
}

CliAuthenticator::~CliAuthenticator()
{
}

Authenticator::Methods CliAuthenticator::availableMethods() const
{
    Authenticator::Methods methods;

    if (m_watcher->lockCodeSet()) {
        methods |= Authenticator::LockCode;
    }

    return methods;
}

Authenticator::Methods CliAuthenticator::authenticate(
        const QString &authenticator, const QVariant &challengeCode, Authenticator::Methods methods)
{
    auto connection = QDBusContext::connection().name();

    if (!m_authenticatorPath.isEmpty()) {
        clearConnection();

        authenticationEnded(false);
    }

    if (m_watcher->lockCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = m_attemptCount.value(0).toInt();

        if (maximum > 0 && attempts >= maximum) {
            QDBusContext::sendErrorReply(QStringLiteral("org.nemomobile.devicelock.Authenticator.LockedOut"));
            return Authenticator::Methods();
        } else {
            m_authenticatorConnection = connection;
            m_authenticatorPath = authenticator;

            return authenticationStarted(challengeCode, methods);
        }
    } else {
        // No code is set. Authenticate immediately with a dummy lock code.

        // Send the reply first then follow up with the authenticated message.
        QDBusContext::setDelayedReply(true);
        QDBusContext::connection().send(QDBusContext::message().createReply(marshallArguments(uint(0))));

        sendAuthenticated(connection, authenticator, QStringLiteral("12345"));
        return Authenticator::Methods();
    }
}

Authenticator::Methods CliAuthenticator::authenticationStarted(const QVariant &, Authenticator::Methods methods)
{
    return methods & Authenticator::LockCode;
}

void CliAuthenticator::enterLockCode(const QString &authenticator, const QString &code)
{
    auto connection = QDBusContext::connection().name();

    if (!checkConnection(connection, authenticator)) {
        return;
    } else if (PluginCommand *command = m_watcher->checkCode(this, code)) {
        command->onSuccess([this, connection, authenticator, code]() {
            // Check the connection hasn't been cancelled in the interim.
            if (checkConnection(connection, authenticator)) {
                lockCodeValidated(code);
            }
        });

        command->onFailure([this, connection, authenticator]() {
            const int maximum = maximumAttempts();

            if (maximum > 0) {
                const int attempts = m_attemptCount.value(0).toInt() + 1;
                m_attemptCount.set(attempts);

                if (checkConnection(connection, authenticator)) {
                    sendFeedback(
                                connection,
                                authenticator,
                                Authenticator::IncorrectLockCode,
                                qMax(0, maximum - attempts));

                    if (attempts >= maximum) {
                        sendError(connection, authenticator, Authenticator::LockedOut);

                        clearConnection();

                        authenticationEnded(false);
                    }
                }
            } else if (checkConnection(connection, authenticator)) {
                sendFeedback(connection, authenticator, Authenticator::IncorrectLockCode, -1);
            }
        });
    } else {
        QDBusContext::sendErrorReply(QDBusError::InternalError);
    }
}

void CliAuthenticator::lockCodeValidated(const QString &lockCode)
{
    confirmAuthentication(lockCode);
}

void CliAuthenticator::confirmAuthentication(const QVariant &authenticationToken)
{
    m_attemptCount.set(0);

    sendAuthenticated(m_authenticatorConnection, m_authenticatorPath, authenticationToken);
    clearConnection();

    authenticationEnded(true);
}

void CliAuthenticator::abortAuthentication(Authenticator::Error error)
{
    sendError(m_authenticatorConnection, m_authenticatorPath, error);
    clearConnection();

    authenticationEnded(false);
}

void CliAuthenticator::cancel(const QString &authenticator)
{
    if (checkConnection(QDBusContext::connection().name(), authenticator)) {
        clearConnection();

        authenticationEnded(false);
    }
}

void CliAuthenticator::authenticationEnded(bool)
{
}

void CliAuthenticator::clientDisconnected(const QString &connectionName)
{
    if (m_authenticatorConnection == connectionName) {
        clearConnection();

        authenticationEnded(false);
    }

    HostAuthenticator::clientDisconnected(connectionName);
}

bool CliAuthenticator::checkConnection(const QString &connection, const QString &path)
{
    return m_authenticatorConnection == connection && m_authenticatorPath == path;
}

void CliAuthenticator::clearConnection()
{
    m_authenticatorConnection.clear();
    m_authenticatorPath.clear();
}

}
