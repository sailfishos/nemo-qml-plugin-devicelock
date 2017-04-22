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

#include "devicelock.h"

#include "settingswatcher.h"

namespace NemoDeviceLock
{

/*!
    \class NemoDeviceLock::DeviceLock
    \brief The DeviceLock class provides an interface to the security daemons device lock state.

    Through this interface the current state of the device lock can be queried, and a request to
    release the device lock may be made.  Unlocking requires user authentication which the
    security daemon will independently request through the currently registered device lock
    AuthenticationInput provider.
*/

/*!
    Constructs a device lock interface instance which is a child of \a parent.
*/

DeviceLock::DeviceLock(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this, QStringLiteral("/devicelock/lock"), QStringLiteral("org.nemomobile.devicelock.DeviceLock"))
    , m_settings(SettingsWatcher::instance())
    , m_state(Undefined)
    , m_enabled(true)
    , m_unlocking(false)
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &DeviceLock::automaticLockingChanged);
    connect(this, &DeviceLock::enabledChanged,
            this, &DeviceLock::automaticLockingChanged);
    connect(m_settings.data(), &SettingsWatcher::showNotificationsChanged,
            this, &DeviceLock::showNotificationsChanged);
    connect(this, &DeviceLock::stateChanged,
            this, &DeviceLock::showNotificationsChanged);

    m_connection->onConnected(this, [this] {
        connected();
    });
    m_connection->onDisconnected(this, [this] {
        const bool wasUnlocked = m_state == Unlocked;

        m_state = Undefined;

        if (m_unlocking) {
            m_unlocking = false;
            emit unlockingChanged();
        }

        emit stateChanged();

        if (wasUnlocked) {
            emit locked();
        }
    });
    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys a device lock interface instance.
*/

DeviceLock::~DeviceLock()
{
}

/*!
    \property NemoDeviceLock::DeviceLock::automaticLocking

    This property holds the number of minutes of inactivity required before the device will
    automatically lock.
*/

int DeviceLock::automaticLocking() const
{
    return isEnabled() ? m_settings->automaticLocking : -1;
}

/*!
    \property NemoDeviceLock::DeviceLock::showNotifications

    This property holds whether the notifications should be shown over the lockscreen.
*/

bool DeviceLock::showNotifications() const
{
    switch (m_state) {
    case Unlocked:
        return true;
    case Locked:
        return m_settings->showNotifications > 0;
    default:
        return false;
    }
}

/*!
    \property NemoDeviceLock::DeviceLock::enabled

    This property holds whether device lock is enabled.
*/

bool DeviceLock::isEnabled() const
{
    return m_enabled;
}

/*!
    \property NemoDeviceLock::DeviceLock::unlocking

    This property holds whether the user is currently being prompted for authentication to unlock
    the device.
*/

bool DeviceLock::isUnlocking() const
{
    return m_unlocking;
}

/*!
    \property NemoDeviceLock::DeviceLock::state

    This property holds the current state of the device lock.
*/

DeviceLock::LockState DeviceLock::state() const
{
    return m_state;
}

/*!
    Requests user authentication to unlock the device.
*/

void DeviceLock::unlock()
{
    if (!m_unlocking && m_state >= Locked && m_state < Undefined) {
        m_unlocking = true;

        const auto response = call(QStringLiteral("Unlock"));
        response->onError([this](const QDBusError &) {
            m_unlocking = false;
            emit unlockingChanged();
        });

        emit unlockingChanged();
    }
}

/*!
    Cancels a request for user authentication to unlock the device.
*/

void DeviceLock::cancel()
{
    if (m_unlocking) {
        m_unlocking = false;

        call(QStringLiteral("Cancel"));

        emit unlockingChanged();
    }
}

/*!
    \signal NemoDeviceLock::DeviceLock::locked()

    Signals that the device has been locked.
*/

/*!
    \signal NemoDeviceLock::DeviceLock::unlocked()

    Signals that the device has been unlocked.
*/

/*!
    \signal NemoDeviceLock::DeviceLock::unlockError()

    Signals that there was an error requesting authentication to unlock the device.
*/

void DeviceLock::connected()
{
    registerObject();

    subscribeToProperty<bool>(QStringLiteral("Enabled"), [this](bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            emit enabledChanged();
        }
    });
    subscribeToProperty<bool>(QStringLiteral("Unlocking"), [this](bool unlocking) {
        if (m_unlocking != unlocking) {
            m_unlocking = unlocking;
            emit unlockingChanged();
        }
    });
    subscribeToProperty<uint>(QStringLiteral("State"), [this](uint state) {
        if (m_state != state) {
            bool wasLocked = m_state >= Locked;

            m_state = LockState(state);

            emit stateChanged();

            if (!wasLocked && m_state >= Locked && m_state < Undefined) {
                emit locked();
            } else if (wasLocked && m_state == Unlocked) {
                emit unlocked();
            }
        }
    });
}

}
