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

#include "nemoauthenticator.h"

#include "lockcodewatcher.h"

#include <QVariant>

NemoAuthenticator::NemoAuthenticator(QObject *parent)
    : Authenticator(parent)
    , m_watcher(LockCodeWatcher::instance())
    , m_attemptCount(QStringLiteral("/desktop/nemo/devicelock/attempt_count"))
    , m_utilizedMethods()
    , m_authenticating(false)
{
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &NemoAuthenticator::availableMethodsChanged);
}

NemoAuthenticator::~NemoAuthenticator()
{
}

Authenticator::Methods NemoAuthenticator::availableMethods() const
{
    Methods methods;

    if (m_watcher->lockCodeSet()) {
        methods |= LockCode;
    }

    return methods;
}

Authenticator::Methods NemoAuthenticator::utilizedMethods() const
{
    return m_utilizedMethods;
}

void NemoAuthenticator::setUtilizedMethods(Methods methods)
{
    if (m_utilizedMethods != methods) {
        m_utilizedMethods = methods;

        emit utilizedMethodsChanged();
    }
}

bool NemoAuthenticator::isAuthenticating() const
{
    return m_authenticating;
}

void NemoAuthenticator::authenticate(const QVariant &, Methods methods)
{
    if (m_watcher->lockCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = m_attemptCount.value(0).toInt();

        if (maximum > 0 && attempts >= maximum) {
            emit error(LockedOut);
        } else {
            m_authenticating = true;

            authenticationStarted(methods);

            emit authenticatingChanged();
        }
    } else {
        // No code is set. authenticate immediately with a dummy lock code.
        emit authenticated(QStringLiteral("12345"));
    }
}


void NemoAuthenticator::enterLockCode(const QString &code)
{
    if (!m_authenticating) {
        return;
    }

    if (m_watcher->checkCode(code)) {
        confirmAuthentication(code);
    } else {
        const int maximum = maximumAttempts();

        if (maximum > 0) {
            const int attempts = m_attemptCount.value(0).toInt() + 1;
            m_attemptCount.set(attempts);

            emit feedback(IncorrectLockCode, qMax(0, maximum - attempts));

            if (attempts >= maximum) {
                m_authenticating = false;
                m_utilizedMethods = Methods();

                authenticationEnded(false);

                emit error(LockedOut);
                emit authenticatingChanged();
                emit utilizedMethodsChanged();
            }
        } else {
            emit feedback(IncorrectLockCode, -1);
        }
    }
}

void NemoAuthenticator::cancel()
{
    m_authenticating = false;
    m_utilizedMethods = Methods();

    authenticationEnded(false);

    emit authenticatingChanged();
    emit utilizedMethodsChanged();
}

void NemoAuthenticator::authenticationStarted(Methods methods)
{
    setUtilizedMethods(methods & LockCode);
}

void NemoAuthenticator::authenticationEnded(bool)
{
}

void NemoAuthenticator::confirmAuthentication(const QVariant &authenticationToken)
{
    m_authenticating = false;

    m_attemptCount.set(0);

    authenticationEnded(true);

    emit authenticated(authenticationToken);
    emit authenticatingChanged();
}
