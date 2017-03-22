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
    , m_lockState(DeviceLock::Undefined)
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &HostDeviceLock::automaticLockingChanged);
}

HostDeviceLock::~HostDeviceLock()
{
}

DeviceLock::LockState HostDeviceLock::state() const
{
    return m_lockState;
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
        setLocked(false);
        return;
    case CanAuthenticate:
        authenticationStarted(
                    Authenticator::SecurityCode | Authenticator::Fingerprint,
                    AuthenticationInput::EnterSecurityCode);
        break;
    case CanAuthenticateSecurityCode:
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterSecurityCode);
        break;
    case SecurityCodeRequired:
        m_state = EnteringNewSecurityCode;
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterNewSecurityCode);
        break;
    case ManagerLocked:
        m_state = AuthenticationError;
        authenticationUnavailable(AuthenticationInput::LockedByManager);
        feedback(AuthenticationInput::ContactSupport, -1);
        break;
    case TemporarilyLocked:
        m_state = AuthenticationError;
        authenticationUnavailable(AuthenticationInput::MaximumAttemptsExceeded);
        feedback(AuthenticationInput::TemporarilyLocked, -1);
        break;
    case PermanentlyLocked:
        m_state = AuthenticationError;
        authenticationUnavailable(AuthenticationInput::MaximumAttemptsExceeded);
        feedback(AuthenticationInput::PermanentlyLocked, -1);
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
            feedback(AuthenticationInput::EnterNewSecurityCode, -1);
            break;
        case SecurityCodeInHistory:
            break;
        case LockedOut:
            lockedOut();
            break;
        default: {
            const int maximum = maximumAttempts();

            if (maximum > 0) {
                feedback(AuthenticationInput::IncorrectSecurityCode, qMax(0, maximum - result));

                if (result >= maximum) {
                    lockedOut();
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
            m_newCode.clear();

            feedback(AuthenticationInput::SecurityCodesDoNotMatch, -1);

            switch (availability()) {
            case AuthenticationNotRequired:
            case SecurityCodeRequired:
                m_state = EnteringNewSecurityCode;
                feedback(AuthenticationInput::EnterNewSecurityCode, -1);
                break;
            default:
                m_state = Authenticating;
                feedback(AuthenticationInput::EnterSecurityCode, -1);
                break;
            }
            break;
        } else {
            setCodeFinished(setCode(m_currentCode, code));
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
        m_currentCode.clear();
        if (m_state == ChangingSecurityCode || m_state == RepeatingNewSecurityCode) {
            unlockFinished(unlockWithCode(m_newCode));
        } else if (m_state == Canceled) {
            m_state = Idle;

            authenticationEnded(false);

            unlockingChanged();
        } else {
            abortAuthentication(AuthenticationInput::SoftwareError);
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
        m_currentCode.clear();
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

    switch (availability()) {
    case AuthenticationNotRequired:
    case CanAuthenticate:
    case CanAuthenticateSecurityCode:
    case SecurityCodeRequired:
        setLocked(false);

        authenticationEnded(true);
        break;
    case ManagerLocked:
    case TemporarilyLocked:
    case PermanentlyLocked:
        authenticationEnded(false);
        break;
    }

    unlockingChanged();
}

void HostDeviceLock::abortAuthentication(AuthenticationInput::Error error)
{
    m_currentCode.clear();
    m_newCode.clear();

    switch (m_state) {
    case Authenticating:
    case Unlocking:
    case EnteringNewSecurityCode:
    case RepeatingNewSecurityCode:
    case ChangingSecurityCode:
        m_state = AuthenticationError;
        break;
    default:
        break;
    }

    HostAuthenticationInput::abortAuthentication(error);
}

void HostDeviceLock::stateChanged()
{
    const auto previousState = m_lockState;

    switch (availability()) {
    case ManagerLocked:
        m_lockState = DeviceLock::ManagerLockout;
        break;
    case TemporarilyLocked:
        m_lockState = DeviceLock::TemporaryLockout;
        break;
    case PermanentlyLocked:
        m_lockState = DeviceLock::PermanentLockout;
        break;
    case SecurityCodeRequired:
        m_lockState = DeviceLock::Locked;
        break;
    default:
        m_lockState = isLocked()
                ? DeviceLock::Locked
                : DeviceLock::Unlocked;
    }

    if (m_lockState != previousState) {
        propertyChanged(
                    QStringLiteral("org.nemomobile.devicelock.DeviceLock"),
                    QStringLiteral("State"),
                    QVariant::fromValue(uint(m_lockState)));
    }
}

void HostDeviceLock::lockedChanged()
{
    stateChanged();
}

void HostDeviceLock::availabilityChanged()
{
    const auto available = availability();

    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.DeviceLock"),
                QStringLiteral("Enabled"),
                available != AuthenticationNotRequired);

    switch (available) {
    case AuthenticationNotRequired:
        switch (m_state) {
        case Authenticating:
        case AuthenticationError:
            m_state = Idle;
            setLocked(false);
            break;
        default:
            break;
        }
        break;
    case CanAuthenticate:
        switch (m_state) {
        case AuthenticationError:
            m_state = Authenticating;
            authenticationResumed(AuthenticationInput::EnterSecurityCode);
            break;
        default:
            break;
        }
        break;
    case CanAuthenticateSecurityCode:
        switch (m_state) {
        case Authenticating:
            feedback(AuthenticationInput::EnterSecurityCode, -1, Authenticator::SecurityCode);
            break;
        case AuthenticationError:
            m_state = Authenticating;
            authenticationResumed(AuthenticationInput::EnterSecurityCode, Authenticator::SecurityCode);
            break;
        default:
            break;
        }
        break;
    case SecurityCodeRequired:
        switch (m_state) {
        case Authenticating:
            m_state = EnteringNewSecurityCode;
            feedback(AuthenticationInput::EnterNewSecurityCode, -1, Authenticator::SecurityCode);
            break;
        case AuthenticationError:
            m_state = EnteringNewSecurityCode;
            authenticationResumed(AuthenticationInput::EnterNewSecurityCode, Authenticator::SecurityCode);
            break;
        default:
            break;
        }
        break;
    case ManagerLocked:
        setLocked(true);

        switch (m_state) {
        case Authenticating:
        case EnteringNewSecurityCode:
        case RepeatingNewSecurityCode:
        case AuthenticationError:
            abortAuthentication(AuthenticationInput::LockedByManager);
            feedback(AuthenticationInput::ContactSupport, -1);
            break;
        default:
            break;
        }
        break;
    case TemporarilyLocked:
        setLocked(true);

        switch (m_state) {
        case Authenticating:
        case EnteringNewSecurityCode:
        case RepeatingNewSecurityCode:
        case AuthenticationError:
            abortAuthentication(AuthenticationInput::MaximumAttemptsExceeded);
            feedback(AuthenticationInput::TemporarilyLocked, -1);
            break;
        default:
            break;
        }
        break;
    case PermanentlyLocked:
        setLocked(true);

        switch (m_state) {
        case Authenticating:
        case EnteringNewSecurityCode:
        case RepeatingNewSecurityCode:
        case AuthenticationError:
            abortAuthentication(AuthenticationInput::MaximumAttemptsExceeded);
            feedback(AuthenticationInput::PermanentlyLocked, -1);
            break;
        default:
            break;
        }
        break;
    }

    stateChanged();
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
