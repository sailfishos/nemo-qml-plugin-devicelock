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

#include "logging.h"

namespace NemoDeviceLock
{

AuthenticatorAdaptor::AuthenticatorAdaptor(Authenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

void AuthenticatorAdaptor::Authenticated(const QDBusVariant &authenticationToken)
{
    m_authenticator->handleAuthentication(authenticationToken.variant());
}

void AuthenticatorAdaptor::Aborted()
{
    m_authenticator->handleAborted();
}

/*!
    \class NemoDeviceLock::Authenticator
    \brief The Authenticator class provides an interface to request the user authenticate themselves.

    APIs which require user authorization to perform a task will issue a challenge code to be passed
    to the authenticator, if the user successfully identifies themselves to the security daemon the
    authenticated() signal will be emitted with an authentication token which may be used by the
    issuing a API in combination with the challenge code to verify the authentication.
*/

/*!
    Constructs a new authenticator which is a child of \a parent.
*/

Authenticator::Authenticator(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/authenticator"),
          QStringLiteral("org.nemomobile.devicelock.Authenticator"))
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_availableMethods()
    , m_authenticating(false)
{
    m_connection->onConnected(this, [this] {
        connected();
    });

    m_connection->onDisconnected(this, [this] {
        handleAborted();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys an authenticator.
*/

Authenticator::~Authenticator()
{
}

/*!
    \qmlproperty NemoDeviceLock::Authenticator::availableMethods

    This property holds the authentication methods that may currently be utilized.
*/

Authenticator::Methods Authenticator::availableMethods() const
{
    return m_availableMethods;
}

/*!
    \qmlproperty NemoDeviceLock::Authenticator::authenticating

    This property holds whether the the authenticator is currently authenticating a challenge code.
*/

bool Authenticator::isAuthenticating() const
{
    return m_authenticating;
}

/*!
    Requests the authentication of a \a challengeCode using any of the supplied \a methods.
*/

void Authenticator::authenticate(const QVariant &challengeCode, Methods methods)
{
    const auto response = call(QStringLiteral("Authenticate"), m_localPath, challengeCode, uint(methods));

    m_authenticating = true;

    response->onError([this](const QDBusError &) {
        m_authenticating = false;

        emit aborted();
        emit authenticatingChanged();
    });

    emit authenticatingChanged();
}

/*!
    Cancels an active authentication request.
*/

void Authenticator::cancel()
{
    if (m_authenticating) {
        m_authenticating = false;

        call(QStringLiteral("Cancel"), m_localPath);

        emit authenticatingChanged();
    }
}

/*!
    \signal NemoDeviceLock::Authenticator::authenticated(const QVariant &authenticationToken)

    Signals that the user has successfully authenticated themselves.  The \a authenticationToken
    may be passed to the API which required authentication as proof.
*/

void Authenticator::handleAuthentication(const QVariant &authenticationToken)
{
    if (m_authenticating) {
        m_authenticating = false;

        emit authenticated(authenticationToken);
        emit authenticatingChanged();
    }
}

/*!
    \signal NemoDeviceLock::Authenticator::aborted()

    Signals that the user abandoned a request for authentication.
*/

void Authenticator::handleAborted()
{
    if (m_authenticating) {
        m_authenticating = false;

        qCDebug(devicelock, "Authentication aborted.");

        emit aborted();
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

}
