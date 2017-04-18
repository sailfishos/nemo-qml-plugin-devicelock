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

#ifndef NEMODEVICELOCK_HOSTDEVICELOCK_H
#define NEMODEVICELOCK_HOSTDEVICELOCK_H

#include <nemo-devicelock/devicelock.h>
#include <nemo-devicelock/host/hostauthenticationinput.h>
#include <nemo-devicelock/host/hostobject.h>

#include <QDBusVariant>

namespace NemoDeviceLock
{

class HostDeviceLock;
class HostDeviceLockAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(uint State READ state)
    Q_PROPERTY(bool Enabled READ isEnabled)
    Q_PROPERTY(bool Unlocking READ isUnlocking)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.DeviceLock")
public:
    explicit HostDeviceLockAdaptor(HostDeviceLock *deviceLock);

    uint state() const;
    bool isEnabled() const;
    bool isUnlocking() const;

public slots:
    void Unlock();
    void Cancel();

private:
    HostDeviceLock * const m_deviceLock;
};

class SettingsWatcher;

class HostDeviceLock : public HostAuthenticationInput
{
    Q_OBJECT
public:
    explicit HostDeviceLock(Authenticator::Methods supportedMethods, QObject *parent = nullptr);
    ~HostDeviceLock();

    DeviceLock::LockState state() const;

    bool isUnlocking() const;

    virtual int automaticLocking() const;

    void unlock();
    void enterSecurityCode(const QString &code) override;
    void requestSecurityCode() override;
    void cancel() override;

    Availability availability() const override = 0;
    int checkCode(const QString &code) override = 0;
    int setCode(const QString &oldCode, const QString &newCode) override = 0;

    virtual int unlockWithCode(const QString &code) = 0;

    virtual bool isLocked() const = 0;
    virtual void setLocked(bool locked) = 0;

    void confirmAuthentication() override;
    void abortAuthentication(AuthenticationInput::Error error) override;

    void lockedChanged();
    void availabilityChanged();

    virtual void automaticLockingChanged();

    void unlockFinished(int result);
    void setCodeFinished(int result);

    // Signals
    void notice(DeviceLock::Notice notice, const QVariantMap &data);

protected:
    virtual void stateChanged();

private:
    friend class HostDeviceLockAdaptor;

    enum State {
        Idle,
        Authenticating,
        Unlocking,
        EnteringNewSecurityCode,
        ExpectingGeneratedSecurityCode,
        RepeatingNewSecurityCode,
        ChangingSecurityCode,
        Canceled,
        AuthenticationError
    };

    inline bool isEnabled() const;
    inline void unlockingChanged();
    inline QVariantMap generatedCodeData();
    inline void enterCodeChangeState(
            FeedbackFunction feedback, Authenticator::Methods methods = Authenticator::Methods());

    HostDeviceLockAdaptor m_adaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    QString m_currentCode;
    QString m_newCode;
    QString m_generatedCode;
    State m_state;
    DeviceLock::LockState m_lockState;
};

}

#endif
