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

#include "lockcodewatcher.h"

#include <QDBusMessage>

namespace NemoDeviceLock
{

CliAuthenticator::CliAuthenticator(QObject *parent)
    : HostAuthenticator(Authenticator::SecurityCode, parent)
    , m_watcher(LockCodeWatcher::instance())
{
    connect(m_watcher.data(), &LockCodeWatcher::securityCodeSetChanged,
            this, &CliAuthenticator::availableMethodsChanged);
    connect(m_watcher.data(), &LockCodeWatcher::securityCodeSetChanged,
            this, &CliAuthenticator::availabilityChanged);
}

CliAuthenticator::~CliAuthenticator()
{
}

Authenticator::Methods CliAuthenticator::availableMethods() const
{
    Authenticator::Methods methods;

    if (m_watcher->securityCodeSet()) {
        methods |= Authenticator::SecurityCode;
    }

    return methods;
}


HostAuthenticationInput::Availability CliAuthenticator::availability() const
{
    if (m_watcher->securityCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = currentAttempts();

        if (maximum > 0 && attempts >= maximum) {
            return PermanentlyLocked;
        } else {
            return CanAuthenticate;
        }
    } else {
        return AuthenticationNotRequired;
    }
}

int CliAuthenticator::checkCode(const QString &code)
{
    return m_watcher->runPlugin(QStringList() << QStringLiteral("--check-code") << code);
}

int CliAuthenticator::setCode(const QString &oldCode, const QString &newCode)
{
    return m_watcher->runPlugin(QStringList() << QStringLiteral("--set-code") << oldCode << newCode);
}

bool CliAuthenticator::clearCode(const QString &code)
{
    return m_watcher->runPlugin(QStringList() << QStringLiteral("--clear-code") << code) == Success;
}

void CliAuthenticator::enterSecurityCode(const QString &code)
{
    m_securityCode = code;
    HostAuthenticator::enterSecurityCode(code);
    m_securityCode.clear();
}

QVariant CliAuthenticator::authenticateChallengeCode(const QVariant &)
{
    return m_securityCode;
}

}
