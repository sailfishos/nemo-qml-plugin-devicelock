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

#include "hostauthorization.h"

#include <QDBusObjectPath>

namespace NemoDeviceLock
{

static const auto clientInterface = QStringLiteral("org.nemomobile.devicelock.client.Authorization");

HostAuthorizationAdaptor::HostAuthorizationAdaptor(HostAuthorization *authorization)
    : QDBusAbstractAdaptor(authorization)
    , m_authorization(authorization)
{
}

void HostAuthorizationAdaptor::RequestChallenge(
        const QDBusObjectPath &path, uint requestedMethods, uint authenticatingPid)
{
    m_authorization->requestChallenge(path.path(), Authenticator::Methods(requestedMethods), authenticatingPid);
}

void HostAuthorizationAdaptor::RelinquishChallenge(const QDBusObjectPath &path)
{
    m_authorization->relinquishChallenge(path.path());
}

HostAuthorization::HostAuthorization(
        const QString &path, Authenticator::Methods allowedMethods, QObject *parent)
    : HostObject(path, parent)
    , m_adaptor(this)
    , m_allowedMethods(allowedMethods)
{
}

HostAuthorization::~HostAuthorization()
{
}

void HostAuthorization::requestChallenge(const QString &, Authenticator::Methods requestedMethods, uint)
{
    const auto methods = m_allowedMethods & requestedMethods;
    if (methods) {
        QDBusContext::setDelayedReply(true);

        QDBusContext::connection().send(QDBusContext::message().createReply(NemoDBus::marshallArguments(
                    QVariant(0), uint(methods))));
    } else {
        QDBusContext::sendErrorReply(QDBusError::NotSupported);
    }
}

void HostAuthorization::relinquishChallenge(const QString &)
{
    if (!m_allowedMethods) {
        QDBusContext::sendErrorReply(QDBusError::NotSupported);
    }
}

void HostAuthorization::challengeExpired(const QString &connection, const QString &path)
{
    NemoDBus::send(connection, path, clientInterface, QStringLiteral("ChallengeExpired"));
}

}
