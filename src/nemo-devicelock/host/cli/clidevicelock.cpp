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

#include "clidevicelock.h"

#include "lockcodewatcher.h"

#include <QDBusConnection>
#include <QDBusMessage>

namespace NemoDeviceLock
{

CliDeviceLock::CliDeviceLock(QObject *parent)
    : MceDeviceLock(Authenticator::LockCode, parent)
    , m_watcher(LockCodeWatcher::instance())
    , m_unlocking(false)
{
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &CliDeviceLock::enabledChanged);

    init();
}

CliDeviceLock::~CliDeviceLock()
{
}

bool CliDeviceLock::isEnabled() const
{
    return m_watcher->lockCodeSet();
}

bool CliDeviceLock::isUnlocking() const
{
    return m_unlocking;
}

void CliDeviceLock::unlock()
{
    if (m_unlocking || !m_watcher->lockCodeSet() || state() == DeviceLock::Unlocked) {
        return;
    }

    const int maximum = maximumAttempts();
    const int attempts = currentAttempts();

    m_unlocking = true;

    if (maximum > 0 && attempts >= maximum) {
        authenticationUnavailable(AuthenticationInput::LockedOut);
    } else {
        authenticationStarted(Authenticator::LockCode, AuthenticationInput::EnterLockCode);
    }

    unlockingChanged();
}

void CliDeviceLock::enterLockCode(const QString &code)
{
    if (!m_unlocking) {
        return;
    } else if (const auto command = m_watcher->unlock(this, code)) {
        command->onSuccess([this] {
            if (m_unlocking) {
                m_unlocking = false;

                authenticationEnded(true);

                setState(DeviceLock::Unlocked);

                unlockingChanged();
            }
        });
        command->onFailure([this](int exitCode) {
            const int maximum = maximumAttempts();

            if (maximum > 0 && exitCode < 0) {
                const int attempts = -exitCode;
                feedback(AuthenticationInput::IncorrectLockCode, qMax(0, maximum - attempts));

                if (attempts >= maximum) {
                    abortAuthentication(AuthenticationInput::LockedOut);
                }
            } else {
                feedback(AuthenticationInput::IncorrectLockCode, -1);
            }
        });
    } else {
        abortAuthentication(AuthenticationInput::LockedOut);
    }
}

void CliDeviceLock::cancel()
{
    if (m_unlocking) {
        m_unlocking = false;

        authenticationEnded(false);

        unlockingChanged();
    }
}

}
