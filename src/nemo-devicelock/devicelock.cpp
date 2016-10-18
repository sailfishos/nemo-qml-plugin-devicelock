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

DeviceLock::~DeviceLock()
{
}

int DeviceLock::automaticLocking() const
{
    return isEnabled() ? m_settings->automaticLocking : -1;
}

bool DeviceLock::isEnabled() const
{
    return m_enabled;
}

bool DeviceLock::isUnlocking() const
{
    return m_unlocking;
}

DeviceLock::LockState DeviceLock::state() const
{
    return m_state;
}

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

void DeviceLock::cancel()
{
    if (m_unlocking) {
        m_unlocking = false;

        call(QStringLiteral("Cancel"));

        emit unlockingChanged();
    }
}

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
