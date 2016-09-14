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

#ifndef NEMODEVICELOCK_HOSTAUTHENTICATOR_H
#define NEMODEVICELOCK_HOSTAUTHENTICATOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QDBusVariant>

#include <nemo-devicelock/authenticator.h>

#include <nemo-devicelock/host/hostobject.h>

QT_BEGIN_NAMESPACE
class QDBusConnection;
QT_END_NAMESPACE

namespace NemoDeviceLock
{

class HostAuthenticator;
class HostAuthenticatorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(uint AvailableMethods READ availableMethods)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.Authenticator")
public:
    explicit HostAuthenticatorAdaptor(HostAuthenticator *authenticator);

    uint availableMethods() const;

public slots:
    uint Authenticate(const QDBusObjectPath &path, const QDBusVariant &challengeCode, uint methods);
    void EnterLockCode(const QDBusObjectPath &path, const QString &lockCode);
    void Cancel(const QDBusObjectPath &path);

private:
    HostAuthenticator *m_authenticator;
};

class HostAuthenticator : public HostObject
{
    Q_OBJECT
public:
    explicit HostAuthenticator(QObject *parent = nullptr);
    ~HostAuthenticator();

protected:
    virtual Authenticator::Methods availableMethods() const = 0;
    virtual Authenticator::Methods authenticate(
            const QString &authenticator,
            const QVariant &challengeCode,
            Authenticator::Methods methods) = 0;
    virtual void enterLockCode(const QString &authenticator, const QString &code) = 0;
    virtual void cancel(const QString &authenticator) = 0;

    int maximumAttempts() const;

    void sendAuthenticated(
            const QString &connectionName, const QString &path, const QVariant &authenticationToken);
    void sendFeedback(
            const QString &connectionName,
            const QString &path,
            Authenticator::Feedback feedback,
            int attemptsRemaining,
            Authenticator::Methods utilizedMethods = Authenticator::Methods());
    void sendError(const QString &connection, const QString &path, Authenticator::Error error);

    void availableMethodsChanged();

private:
    friend class HostAuthenticatorAdaptor;

    HostAuthenticatorAdaptor m_adaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
};

}

#endif
