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

#include "hostdevicelock.h"

#include "settingswatcher.h"

namespace NemoDeviceLock
{

HostDeviceLockAdaptor::HostDeviceLockAdaptor(HostDeviceLock *deviceLock)
    : QDBusAbstractAdaptor(deviceLock)
    , m_deviceLock(deviceLock)
{
}

uint HostDeviceLockAdaptor::state() const
{
    return m_deviceLock->state();
}

bool HostDeviceLockAdaptor::isEnabled() const
{
    return m_deviceLock->isEnabled();
}

bool HostDeviceLockAdaptor::isUnlocking() const
{
    return m_deviceLock->isUnlocking();
}

void HostDeviceLockAdaptor::Unlock()
{
    m_deviceLock->unlock();
}

void HostDeviceLockAdaptor::Cancel()
{
    m_deviceLock->cancel();
}

HostDeviceLock::HostDeviceLock(Authenticator::Methods supportedMethods, QObject *parent)
    : HostAuthenticationInput(QStringLiteral("/devicelock/lock"), supportedMethods, parent)
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_state(Idle)
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &HostDeviceLock::automaticLockingChanged);
}

HostDeviceLock::~HostDeviceLock()
{
}

int HostDeviceLock::automaticLocking() const
{
    return isEnabled() ? m_settings->automaticLocking : -1;
}

bool HostDeviceLock::isEnabled() const
{
    return availability() != AuthenticationNotRequired;
}

bool HostDeviceLock::isUnlocking() const
{
    return m_state != Idle;
}

void HostDeviceLock::unlock()
{
    if (m_state != Idle || state() == DeviceLock::Unlocked) {
        return;
    }

    m_state = Authenticating;

    switch (availability()) {
    case AuthenticationNotRequired:
        m_state = Idle;
        setState(DeviceLock::Unlocked);
        return;
    case CanAuthenticate:
        authenticationStarted(
                    Authenticator::SecurityCode | Authenticator::Fingerprint,
                    AuthenticationInput::EnterSecurityCode);
        break;
    case CanAuthenticateSecurityCode:
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterSecurityCode);
        break;
    case AuthenticationLocked:
        authenticationUnavailable(AuthenticationInput::LockedOut);
        break;
    }

    unlockingChanged();
}

void HostDeviceLock::enterSecurityCode(const QString &code)
{
    switch (m_state) {
    case Idle:
        break;
    case Authenticating: {
        switch (const int result = checkCode(code)) {
        case Success:
            unlockFinished(unlockWithCode(code));
            break;
        case SecurityCodeExpired:
            m_state = EnteringNewSecurityCode;
            m_currentCode = code;
            feedback(AuthenticationInput::SecurityCodeExpired, -1);
            break;
        case SecurityCodeInHistory:
            break;
        case LockedOut:
            abortAuthentication(AuthenticationInput::LockedOut);
            break;
        default: {
            const int maximum = maximumAttempts();

            if (maximum > 0) {
                feedback(AuthenticationInput::IncorrectSecurityCode, qMax(0, maximum - result));

                if (result >= maximum) {
                    abortAuthentication(AuthenticationInput::LockedOut);
                }
            } else {
                feedback(AuthenticationInput::IncorrectSecurityCode, -1);
            }
            break;
        }
        }
        break;
    }
    case EnteringNewSecurityCode:
        m_newCode = code;
        m_state = RepeatingNewSecurityCode;
        feedback(AuthenticationInput::RepeatNewSecurityCode, -1);
        break;
    case RepeatingNewSecurityCode:
        if (m_newCode != code) {
            m_currentCode.clear();

            m_state = Authenticating;
            feedback(AuthenticationInput::SecurityCodesDoNotMatch, -1);
            feedback(AuthenticationInput::EnterNewSecurityCode, -1);
        } else {
            // With disk encryption enabled changing the code can take a few seconds, don't leave
            // the user hanging.

            const auto currentCode = m_currentCode;
            m_currentCode.clear();
            m_newCode.clear();

            setCodeFinished(setCode(currentCode, code));
        }
        break;
    case Unlocking:
    case ChangingSecurityCode:
    case Canceled:
    case AuthenticationError:
        break;
    }
}

void HostDeviceLock::unlockFinished(int result)
{
    switch (result) {
    case Success:
        confirmAuthentication();
        break;
    case Evaluating:
        if (m_state == Authenticating) {
            m_state = Unlocking;
            authenticationEvaluating();
        } else if (m_state == ChangingSecurityCode) {
            m_state = Unlocking;
        } else {
            abortAuthentication(AuthenticationInput::SoftwareError);
        }
        break;
    default:
        if (m_state == Canceled) {
            m_state = Idle;

            authenticationEnded(false);

            unlockingChanged();
        } else {
            abortAuthentication(AuthenticationInput::SoftwareError);
        }
        break;
    }
}

void HostDeviceLock::setCodeFinished(int result)
{
    switch (result) {
    case Success:
        qCDebug(daemon, "Lock code changed.");
        if (m_state == ChangingSecurityCode) {
            unlockFinished(unlockWithCode(m_newCode));
        } else if (m_state == Canceled) {
            m_state = Idle;

            authenticationEnded(false);

            unlockingChanged();
        }
        break;
    case SecurityCodeInHistory:
        qCDebug(daemon, "Security code disallowed.");
        m_state = EnteringNewSecurityCode;
        feedback(AuthenticationInput::SecurityCodeInHistory, -1);
        feedback(AuthenticationInput::EnterNewSecurityCode, -1);
        break;
    case Evaluating:
        if (m_state == RepeatingNewSecurityCode) {
            m_state = ChangingSecurityCode;
            authenticationEvaluating();
        } else {
            abortAuthentication(AuthenticationInput::SoftwareError);
        }
        return;
    default:
        qCDebug(daemon, "Lock code change failed.");
        if (m_state == Canceled) {
            m_state = Idle;

            authenticationEnded(false);

            unlockingChanged();
        } else {
            m_state = AuthenticationError;
            authenticationUnavailable(AuthenticationInput::SoftwareError);
        }
        break;
    }
    m_newCode.clear();
}

void HostDeviceLock::cancel()
{
    if (m_state == Unlocking || m_state == ChangingSecurityCode) {
        m_state = Canceled;
    } else if (m_state != Idle && m_state != Canceled) {
        m_state = Idle;

        authenticationEnded(false);

        unlockingChanged();
    }
}

void HostDeviceLock::confirmAuthentication()
{
    m_state = Idle;

    setState(DeviceLock::Unlocked);

    authenticationEnded(true);

    unlockingChanged();
}

void HostDeviceLock::stateChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.DeviceLock"),
                QStringLiteral("State"),
                QVariant::fromValue(uint(state())));
}

void HostDeviceLock::availabilityChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.DeviceLock"),
                QStringLiteral("Enabled"),
                isEnabled());
}

void HostDeviceLock::unlockingChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.DeviceLock"),
                QStringLiteral("Unlocking"),
                isUnlocking());
}

void HostDeviceLock::automaticLockingChanged()
{
}

}
