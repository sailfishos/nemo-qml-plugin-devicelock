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

#include "authenticator.h"
#include "settingswatcher.h"

AuthenticatorAdaptor::AuthenticatorAdaptor(Authenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

void AuthenticatorAdaptor::Authenticated(const QDBusVariant &authenticationToken)
{
    m_authenticator->handleAuthentication(authenticationToken.variant());
}

void AuthenticatorAdaptor::Feedback(uint feedback, uint attemptsRemaining)
{
    m_authenticator->feedback(Authenticator::Feedback(feedback), attemptsRemaining);
}

void AuthenticatorAdaptor::Error(uint error)
{
    m_authenticator->handleError(Authenticator::Error(error));
}

Authenticator::Authenticator(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/authenticator"),
          QStringLiteral("org.nemomobile.devicelock.Authenticator"))
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_availableMethods()
    , m_utilizedMethods()
    , m_authenticating(false)
{
    connect(m_settings.data(), &SettingsWatcher::maximumAttemptsChanged,
            this, &Authenticator::maximumAttemptsChanged);
    connect(m_settings.data(), &SettingsWatcher::inputIsKeyboardChanged,
            this, &Authenticator::codeInputIsKeyboardChanged);

    connect(m_connection.data(), &Connection::connected, this, &Authenticator::connected);
    connect(m_connection.data(), &Connection::disconnected, this, &Authenticator::disconnected);

    if (m_connection->isConnected()) {
        connected();
    }
}

Authenticator::~Authenticator()
{
}

int Authenticator::minimumCodeLength() const
{
    return m_settings->minimumLength;
}

int Authenticator::maximumCodeLength() const
{
    return m_settings->maximumLength;
}
int Authenticator::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

bool Authenticator::codeInputIsKeyboard() const
{
    return m_settings->inputIsKeyboard;
}

Authenticator::Methods Authenticator::availableMethods() const
{
    return m_availableMethods;
}

Authenticator::Methods Authenticator::utilizedMethods() const
{
    return m_utilizedMethods;
}

bool Authenticator::isAuthenticating() const
{
    return m_authenticating;
}

void Authenticator::authenticate(const QVariant &challengeCode, Methods methods)
{
    const auto response = call(QStringLiteral("Authenticate"), m_localPath, challengeCode, uint(methods));

    m_authenticating = true;

    response->onFinished<uint>([this](uint methods) {
        if (m_utilizedMethods != Methods(methods)) {
            m_utilizedMethods = Methods(methods);

            emit utilizedMethodsChanged();
        }
    });

    response->onError([this]() {
        m_authenticating = false;

        emit error(SoftwareError);
        emit authenticatingChanged();
    });

    emit authenticatingChanged();
}

void Authenticator::enterLockCode(const QString &code)
{
    call(QStringLiteral("EnterLockCode"), m_localPath, code);
}

void Authenticator::cancel()
{
    if (m_authenticating) {
        m_authenticating = false;

        call(QStringLiteral("Cancel"), m_localPath);

        emit authenticatingChanged();
    }
}

void Authenticator::handleAuthentication(const QVariant &authenticationToken)
{
    if (m_authenticating) {
        m_authenticating = false;

        emit authenticated(authenticationToken);
        emit authenticatingChanged();
    }
}

void Authenticator::handleError(Error error)
{
    if (m_authenticating) {
        m_authenticating = false;

        emit Authenticator::error(error);
        emit authenticatingChanged();
    }
}

void Authenticator::connected()
{
    registerObject();
    subscribeToProperty<uint>(QStringLiteral("AvailableMethods"), [this](uint methods) {
        if (m_availableMethods != Methods(methods)) {
            m_availableMethods = Methods(methods);
            emit availableMethodsChanged();
        }
    });
}

void Authenticator::disconnected()
{
    handleError(SoftwareError);
}
