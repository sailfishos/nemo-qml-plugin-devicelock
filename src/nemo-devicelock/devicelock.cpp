/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Jonni Rainisto <jonni.rainisto@jollamobile.com>
**
** This file is part of lipstick.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

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

    m_connection->onConnected(this, [this] {
        connected();
    });
    m_connection->onDisconnected(this, [this] {
        m_state = Undefined;

        if (m_unlocking) {
            m_unlocking = false;
            emit unlockingChanged();
        }

        emit stateChanged();
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
    if (!m_unlocking && m_state == Locked) {
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
            m_state = LockState(state);

            emit stateChanged();

            if (m_state == Locked) {
                emit locked();
            } else if (m_state == Unlocked) {
                emit unlocked();
            }
        }
    });
}

}
