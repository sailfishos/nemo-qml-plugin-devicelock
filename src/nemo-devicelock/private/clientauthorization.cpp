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

#include "clientauthorization.h"


#include <QCoreApplication>

namespace NemoDeviceLock
{

ClientAuthorizationAdaptor::ClientAuthorizationAdaptor(
        ClientAuthorization *authorization, QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_authorization(authorization)
{
}

void ClientAuthorizationAdaptor::ChallengeExpired()
{
    m_authorization->handleChallengeExpired();
}

ClientAuthorization::ClientAuthorization(
        const QDBusObjectPath &localPath, const QString &hostPath, QObject *parent)
    : Authorization(parent)
    , ConnectionClient(
          this,
          hostPath,
          QStringLiteral("org.nemomobile.devicelock.Authorization"),
          localPath)
    , m_allowedMethods()
    , m_requestedMethods(Authenticator::AllAvailable)
    , m_authenticatingPid(QCoreApplication::applicationPid())
    , m_status(NoChallenge)
{
    m_connection->onDisconnected(this, [this] {
        handleChallengeExpired();
    });
}

ClientAuthorization::~ClientAuthorization()
{
}

Authenticator::Methods ClientAuthorization::allowedMethods() const
{
    return m_allowedMethods;
}

Authenticator::Methods ClientAuthorization::requestedMethods() const
{
    return m_requestedMethods;
}

void ClientAuthorization::setRequestedMethods(Authenticator::Methods methods)
{
    m_requestedMethods = methods;
}

int ClientAuthorization::authenticatingPid() const
{
    return m_authenticatingPid;
}

void ClientAuthorization::setAuthenticatingPid(int pid)
{
    m_authenticatingPid = pid;
}

Authorization::Status ClientAuthorization::status() const
{
    return m_status ;
}

QVariant ClientAuthorization::challengeCode() const
{
    return m_challengeCode;
}

void ClientAuthorization::requestChallenge()
{
    if (m_status != RequestingChallenge) {
        m_status = RequestingChallenge;

        const auto response = call(
                    QStringLiteral("RequestChallenge"),
                    m_localPath,
                    uint(m_requestedMethods),
                    uint(m_authenticatingPid));

        response->onFinished<QVariant, uint>([this](const QVariant &challengeCode, uint allowedMethods) {
            if (m_status == NoChallenge) {
                // The challenge has been relinquished in the mean time.
                return;
            }

            const bool statusChange = m_status != ChallengeIssued;
            const bool challengeChange = m_challengeCode != challengeCode;
            const bool allowedChange = m_allowedMethods != Authenticator::Methods(allowedMethods);

            m_status = ChallengeIssued;
            m_challengeCode = challengeCode;
            m_allowedMethods = Authenticator::Methods(allowedMethods);

            if (challengeChange) {
                emit challengeCodeChanged();
            }

            if (allowedChange) {
                emit allowedMethodsChanged();
            }

            emit challengeIssued();

            if (statusChange) {
                emit statusChanged();
            }
        });

        response->onError([this](const QDBusError &) {
            if (m_status == RequestingChallenge) {
                m_status = NoChallenge;

                emit challengeDeclined();
                emit statusChanged();
            }
        });
    }
}

void ClientAuthorization::relinquishChallenge()
{
    if (m_status != NoChallenge) {
        m_status = NoChallenge;

        call(QStringLiteral("RelinquishChallenge"), m_localPath);

        emit statusChanged();
    }
}

void ClientAuthorization::handleChallengeExpired()
{
    if (m_status != NoChallenge) {
        m_status = NoChallenge;

        emit challengeExpired();
        emit statusChanged();
    }
}

}
