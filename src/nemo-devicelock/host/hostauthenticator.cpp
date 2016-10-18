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

#include "settingswatcher.h"

namespace NemoDeviceLock
{

static const auto authenticatorInterface = QStringLiteral("org.nemomobile.devicelock.client.Authenticator");
static const auto lockCodeInterface = QStringLiteral("org.nemomobile.devicelock.client.LockCodeSettings");

HostAuthenticatorAdaptor::HostAuthenticatorAdaptor(HostAuthenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

uint HostAuthenticatorAdaptor::availableMethods() const
{
    return m_authenticator->availableMethods();
}

void HostAuthenticatorAdaptor::Authenticate(
        const QDBusObjectPath &path, const QDBusVariant &challengeCode, uint methods)
{
    m_authenticator->authenticate(
                path.path(), challengeCode.variant(), Authenticator::Methods(methods));
}

void HostAuthenticatorAdaptor::Cancel(const QDBusObjectPath &path)
{
     m_authenticator->handleCancel(path.path());
}

HostLockCodeSettingsAdaptor::HostLockCodeSettingsAdaptor(HostAuthenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

bool HostLockCodeSettingsAdaptor::isSet() const
{
    return m_authenticator->isLockCodeSet();
}

void HostLockCodeSettingsAdaptor::Change(const QDBusObjectPath &path, const QDBusVariant &challengeCode)
{
    m_authenticator->handleChangeLockCode(path.path(), challengeCode.variant());
}

void HostLockCodeSettingsAdaptor::CancelChange(const QDBusObjectPath &path)
{
    m_authenticator->handleCancel(path.path());
}

void HostLockCodeSettingsAdaptor::Clear(const QDBusObjectPath &path)
{
    m_authenticator->handleClearLockCode(path.path());
}

void HostLockCodeSettingsAdaptor::CancelClear(const QDBusObjectPath &path)
{
    m_authenticator->handleCancel(path.path());
}

HostAuthenticator::HostAuthenticator(Authenticator::Methods supportedMethods, QObject *parent)
    : HostAuthenticationInput(QStringLiteral("/authenticator"), supportedMethods, parent)
    , m_adaptor(this)
    , m_lockCode(this)
{
}

HostAuthenticator::~HostAuthenticator()
{
}

bool HostAuthenticator::authorizeLockCodeSettings(unsigned long)
{
    return true;
}

void HostAuthenticator::authenticated(const QVariant &authenticationToken)
{
    sendToActiveClient(authenticatorInterface, QStringLiteral("Authenticated"), authenticationToken);
    authenticationEnded(true);
}

void HostAuthenticator::aborted()
{
    sendToActiveClient(authenticatorInterface, QStringLiteral("Aborted"));
    authenticationEnded(false);
}

void HostAuthenticator::lockCodeChanged(const QVariant &authenticationToken)
{
    sendToActiveClient(lockCodeInterface, QStringLiteral("Changed"), authenticationToken);
    authenticationEnded(true);
}

void HostAuthenticator::lockCodeChangeAborted()
{
    sendToActiveClient(lockCodeInterface, QStringLiteral("ChangeAborted"));
    authenticationEnded(false);
}

void HostAuthenticator::lockCodeCleared()
{
    sendToActiveClient(lockCodeInterface, QStringLiteral("Cleared"));
    authenticationEnded(true);
}

void HostAuthenticator::lockCodeClearAborted()
{
    sendToActiveClient(lockCodeInterface, QStringLiteral("ClearAborted"));
    authenticationEnded(false);
}

void HostAuthenticator::availableMethodsChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.Authenticator"),
                QStringLiteral("AvailableMethods"),
                QVariant::fromValue(uint(availableMethods())));
}

void HostAuthenticator::lockCodeSetChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.LockCodeSettings"),
                QStringLiteral("LockCodeSet"),
                QVariant::fromValue(isLockCodeSet()));
}

void HostAuthenticator::handleChangeLockCode(const QString &path, const QVariant &challengeCode)
{
    const auto pid = connectionPid(QDBusContext::connection());
    if (pid == 0 || !authorizeLockCodeSettings(pid)) {
        QDBusContext::sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    changeLockCode(path, challengeCode);
}

void HostAuthenticator::handleClearLockCode(const QString &path)
{
    const auto pid = connectionPid(QDBusContext::connection());
    if (pid == 0 || !authorizeLockCodeSettings(pid)) {
        QDBusContext::sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    clearLockCode(path);
}

void HostAuthenticator::handleCancel(const QString &client)
{
    if (isActiveClient(client)) {
        cancel();
    }
}

}
