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

#include "validator.h"

namespace NemoDeviceLock
{

/*!
    \class NemoDeviceLock::Validator
    \brief The Validator class provides access to settings for device lock.
*/

/*!
    Constructs a new device lock settings instance which is a child of \a parent.
*/

Validator::Validator(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/validator"),
          QStringLiteral("org.nemomobile.devicelock.Validator"))
    , m_authorization(m_localPath, path())
    , m_authorizationAdaptor(&m_authorization, this)
    , m_verifyResponse(nullptr)
{
    m_connection->onConnected(this, [this] {
        connected();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys a device lock settings instance.
*/

Validator::~Validator()
{
}

/*!
    \property NemoDeviceLock::Validator::authorization

    This property provides a means of acquiring authorization to change device lock settings.
*/

Authorization *Validator::authorization()
{
    return &m_authorization;
}


Authenticator::Methods Validator::allowedMethods() const
{
    return m_authorization.allowedMethods();
}

void Validator::setAllowedMethods(Authenticator::Methods methods)
{
    if (m_authorization.requestedMethods() != methods) {
        m_authorization.setRequestedMethods(methods);
        emit allowedMethodsChanged();
    }
}

int Validator::authenticatingPid() const
{
    return m_authorization.authenticatingPid();
}

void Validator::setAuthenticatingPid(int pid)
{
    if (m_authorization.authenticatingPid() != pid) {
        m_authorization.setAuthenticatingPid(pid);
        emit authenticatingPidChanged();
    }
}

bool Validator::isVerifying() const
{
    return m_verifyResponse;
}

void Validator::verifyToken(const QVariant &authenticationToken)
{
    auto response = call(QStringLiteral("VerifyToken"), m_localPath, authenticationToken);
    m_verifyResponse = response;

    response->onFinished([this, response]() {
        if (m_verifyResponse == response) {
            m_verifyResponse = nullptr;
            emit tokenVerified();
            emit verifyingChanged();
        }
    });
    response->onError([this, response](const QDBusError &) {
        if (m_verifyResponse == response) {
            m_verifyResponse = nullptr;
            emit tokenRejected();
            emit verifyingChanged();
        }
    });
}

void Validator::connected()
{
    registerObject();
}

}
