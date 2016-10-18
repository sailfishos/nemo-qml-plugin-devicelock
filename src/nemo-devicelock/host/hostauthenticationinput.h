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

#ifndef NEMODEVICELOCK_HOSTAUTHENTICATIONINPUT_H
#define NEMODEVICELOCK_HOSTAUTHENTICATIONINPUT_H

#include <nemo-devicelock/authenticationinput.h>
#include <nemo-devicelock/host/hostobject.h>

QT_BEGIN_NAMESPACE
class QDBusConnection;
QT_END_NAMESPACE

namespace NemoDeviceLock
{

class HostAuthenticationInput;
class HostAuthenticationInputAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.AuthenticationInput")
public:
    explicit HostAuthenticationInputAdaptor(HostAuthenticationInput *authenticationInput);

public slots:
    void SetRegistered(const QDBusObjectPath &path, bool registered);
    void SetActive(const QDBusObjectPath &path, bool active);
    void EnterLockCode(const QDBusObjectPath &path, const QString &lockCode);
    void Cancel(const QDBusObjectPath &path);

private:
    HostAuthenticationInput * const m_authenticationInput;
};

class HostAuthenticationInput : public HostObject
{
    Q_OBJECT
public:
    explicit HostAuthenticationInput(
            const QString &path,
            Authenticator::Methods supportedMethods = Authenticator::LockCode,
            QObject *parent = nullptr);
    virtual ~HostAuthenticationInput();

    int maximumAttempts() const;
    int currentAttempts() const;

    // AuthenticationInput
    virtual bool authorizeInput(unsigned long pid);

    virtual void enterLockCode(const QString &code) = 0;
    void cancel() override = 0;

    // Client
    virtual void authenticationStarted(
            Authenticator::Methods methods,
            AuthenticationInput::Feedback feedback = AuthenticationInput::EnterLockCode);
    void authenticationUnavailable(AuthenticationInput::Error error);
    void authenticationEvaluating();
    virtual void authenticationEnded(bool confirmed);

    virtual void authenticationActive(Authenticator::Methods methods);
    virtual void authenticationInactive();

    virtual void confirmAuthentication();
    virtual void abortAuthentication(AuthenticationInput::Error error);

    // Signals
    void feedback(
            AuthenticationInput::Feedback feedback,
            int attemptsRemaining,
            Authenticator::Methods utilizedMethods = Authenticator::Methods());

    // Housekeeping
    void clientDisconnected(const QString &connectionName) override;

private:
    friend class HostAuthenticationInputAdaptor;

    struct Input
    {
        Input() {}
        Input(const QString &connection, const QString &path) : connection(connection), path(path) {}

        QString connection;
        QString path;
    };

    inline void handleLockCode(const QString &client, const QString &lockCode);
    inline void handleCancel(const QString &client);

    inline void setRegistered(const QString &path, bool registered);
    inline void setActive(const QString &path, bool active);

    HostAuthenticationInputAdaptor m_adaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    QVector<Input> m_inputStack;
    Authenticator::Methods m_supportedMethods;
    Authenticator::Methods m_activeMethods;
    bool m_authenticating;
};

}

#endif
