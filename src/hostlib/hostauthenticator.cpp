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

#include "hostauthenticator.h"

#include "dbusutilities.h"
#include "settingswatcher.h"

namespace NemoDeviceLock
{

static const auto clientInterface = QStringLiteral("org.nemomobile.devicelock.client.Authenticator");

HostAuthenticatorAdaptor::HostAuthenticatorAdaptor(HostAuthenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

uint HostAuthenticatorAdaptor::availableMethods() const
{
    return m_authenticator->availableMethods();
}

uint HostAuthenticatorAdaptor::Authenticate(
        const QDBusObjectPath &path, const QDBusVariant &challengeCode, uint methods)
{
    return m_authenticator->authenticate(
                path.path(), challengeCode.variant(), Authenticator::Methods(methods));
}

void HostAuthenticatorAdaptor::EnterLockCode(const QDBusObjectPath &path, const QString &lockCode)
{
    m_authenticator->enterLockCode(path.path(), lockCode);
}

void HostAuthenticatorAdaptor::Cancel(const QDBusObjectPath &path)
{
     m_authenticator->cancel(path.path());
}

HostAuthenticator::HostAuthenticator(QObject *parent)
    : HostObject(QStringLiteral("/authenticator"), parent)
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
{
}

HostAuthenticator::~HostAuthenticator()
{
}

int HostAuthenticator::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

void HostAuthenticator::sendAuthenticated(
        const QString &connection, const QString &path, const QVariant &authenticationToken)
{
    send(connection, path, clientInterface, QStringLiteral("Authenticated"), authenticationToken);
}

void HostAuthenticator::sendFeedback(
        const QString &connection,
        const QString &path,
        Authenticator::Feedback feedback,
        int attemptsRemaining,
        Authenticator::Methods utilizedMethods)
{
    send(connection,
                path,
                clientInterface,
                QStringLiteral("Feedback"),
                uint(feedback),
                uint(attemptsRemaining),
                uint(utilizedMethods));
}

void HostAuthenticator::sendError(
        const QString &connection, const QString &path, Authenticator::Error error)
{
    send(connection, path, clientInterface, QStringLiteral("Error"), uint(error));
}

void HostAuthenticator::availableMethodsChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.Authenticator"),
                QStringLiteral("AvailableMethods"),
                QVariant::fromValue(uint(availableMethods())));
}

}
