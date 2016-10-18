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

#include <nemo-devicelock/host/hostauthenticationinput.h>
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
    void Authenticate(const QDBusObjectPath &path, const QDBusVariant &challengeCode, uint methods);
    void Cancel(const QDBusObjectPath &path);

private:
    HostAuthenticator *m_authenticator;
};

class HostLockCodeSettings;
class HostLockCodeSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(bool LockCodeSet READ isSet NOTIFY setChanged)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.LockCodeSettings")
public:
    explicit HostLockCodeSettingsAdaptor(HostAuthenticator *authenticator);

    bool isSet() const;

public slots:
    void Change(const QDBusObjectPath &path, const QDBusVariant &challengeCode);
    void CancelChange(const QDBusObjectPath &path);

    void Clear(const QDBusObjectPath &path);
    void CancelClear(const QDBusObjectPath &path);

signals:
    void setChanged();

private:
    HostAuthenticator * const m_authenticator;
};

class HostAuthenticator : public HostAuthenticationInput
{
    Q_OBJECT
public:
    explicit HostAuthenticator(
            Authenticator::Methods supportedMethods = Authenticator::LockCode, QObject *parent = nullptr);
    ~HostAuthenticator();

protected:
    virtual Authenticator::Methods availableMethods() const = 0;
    virtual bool isLockCodeSet() const = 0;

    virtual void authenticate(
            const QString &authenticator,
            const QVariant &challengeCode,
            Authenticator::Methods methods) = 0;
    virtual void cancel(const QString &authenticator) = 0;

    virtual bool authorizeLockCodeSettings(unsigned long pid);

    virtual void changeLockCode(const QString &path, const QVariant &challengeCode) = 0;
    virtual void clearLockCode(const QString &path) = 0;

    void sendAuthenticated(
            const QString &connectionName, const QString &path, const QVariant &authenticationToken);
    void sendAborted(const QString &connectionName, const QString &path);

    void sendLockCodeChanged(const QString &connectionName, const QString &path, const QVariant &authenticationToken);
    void sendLockCodeChangeAborted(const QString &connectionName, const QString &path);

    void sendLockCodeCleared(const QString &connectionName, const QString &path);
    void sendLockCodeClearAborted(const QString &connectionName, const QString &path);

    void availableMethodsChanged();
    void lockCodeSetChanged();

private:
    inline void handleChangeLockCode(const QString &path, const QVariant &challengeCode);
    inline void handleClearLockCode(const QString &path);

    friend class HostAuthenticatorAdaptor;
    friend class HostLockCodeSettingsAdaptor;

    HostAuthenticatorAdaptor m_adaptor;
    HostLockCodeSettingsAdaptor m_lockCode;
};

}

#endif
