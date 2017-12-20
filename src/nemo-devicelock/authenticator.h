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

#ifndef NEMODEVICELOCK_AUTHENTICATOR_H
#define NEMODEVICELOCK_AUTHENTICATOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QSharedDataPointer>

#include <nemo-devicelock/global.h>
#include <nemo-devicelock/private/connection.h>

namespace NemoDeviceLock {

class Authenticator;
class AuthenticatorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.client.Authenticator")
public:
    explicit AuthenticatorAdaptor(Authenticator *authenticator);

public slots:
    Q_NOREPLY void Authenticated(const QDBusVariant &authenticationToken);
    Q_NOREPLY void Aborted();

private:
    Authenticator *m_authenticator;
};

class SettingsWatcher;
class NEMODEVICELOCK_EXPORT Authenticator : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(Methods availableMethods READ availableMethods NOTIFY availableMethodsChanged)
    Q_ENUMS(Method)
    Q_FLAGS(Methods)
public:
    enum Method {
        NoAuthentication    = 0x00,
        SecurityCode        = 0x01,
        Fingerprint         = 0x02,
        AllAvailable = SecurityCode | Fingerprint
    };

    Q_DECLARE_FLAGS(Methods, Method)

    explicit Authenticator(QObject *parent = nullptr);
    ~Authenticator();

    Methods availableMethods() const;
    bool isAuthenticating() const;

    Q_INVOKABLE void authenticate(
            const QVariant &challengeCode, Methods methods = AllAvailable);
    Q_INVOKABLE void cancel();

signals:
    void availableMethodsChanged();
    void authenticatingChanged();

    void authenticated(const QVariant &authenticationToken);
    void aborted();

private:
    friend class AuthenticatorAdaptor;

    inline void connected();

    inline void handleAuthentication(const QVariant &authenticationToken);
    inline void handleAborted();

    AuthenticatorAdaptor m_adaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    Methods m_availableMethods;
    bool m_authenticating;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(NemoDeviceLock::Authenticator::Methods)

#endif
