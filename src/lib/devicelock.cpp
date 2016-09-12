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

DeviceLock::DeviceLock(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this, QStringLiteral("/devicelock/lock"), QStringLiteral("org.nemomobile.devicelock.DeviceLock"))
    , m_authorization(m_localPath, m_remotePath)
    , m_authorizationAdaptor(&m_authorization, this)
    , m_settings(SettingsWatcher::instance())
    , m_state(Undefined)
    , m_enabled(true)
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &DeviceLock::automaticLockingChanged);
    connect(this, &DeviceLock::enabledChanged,
            this, &DeviceLock::automaticLockingChanged);

    connect(m_connection.data(), &Connection::connected, this, &DeviceLock::connected);

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

DeviceLock::LockState DeviceLock::state() const
{
    return m_state;
}

Authorization *DeviceLock::authorization()
{
    return &m_authorization;
}

void DeviceLock::unlock(const QVariant &authenticationToken)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        call(QStringLiteral("Unlock"), m_localPath, authenticationToken);
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

    subscribeToProperty<uint>(QStringLiteral("State"), [this](uint state) {
        if (m_state != state) {
            const bool lock = m_state == Unlocked && state == Locked;
            const bool unlock = m_state == Locked && state == Unlocked;

            m_state = LockState(state);

            emit stateChanged();

            if (lock) {
                emit locked();
            } else if (unlock) {
                emit unlocked();
            }
        }
    });
}
