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

#ifndef NEMODEVICELOCK_DEVICELOCK_H
#define NEMODEVICELOCK_DEVICELOCK_H

#include <QSharedDataPointer>

#include <nemo-devicelock/global.h>
#include <nemo-devicelock/private/connection.h>

namespace NemoDeviceLock
{

class SettingsWatcher;
class NEMODEVICELOCK_EXPORT DeviceLock : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_ENUMS(LockState)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool unlocking READ isUnlocking NOTIFY unlockingChanged)
    Q_PROPERTY(LockState state READ state NOTIFY stateChanged)
    Q_PROPERTY(int automaticLocking READ automaticLocking NOTIFY automaticLockingChanged)
    Q_PROPERTY(bool showNotifications READ showNotifications NOTIFY showNotificationsChanged)
public:
    explicit DeviceLock(QObject *parent = nullptr);
    ~DeviceLock();

    enum LockState
    {
        Unlocked = 0,           /*!< Unlocked - The lock is unlocked */
        Locked,                 /*!< Locked - The lock is being used */
        ManagerLockout,         /*!< ManagerLockout - Access has been restricted by a device manager. */
        TemporaryLockout,       /*!< TemporaryLockout - Access has been temporarily restricted because of excessive incorrect unlock attempts. */
        PermanentLockout,       /*!< PermanentLockout - Access has been permanently restricted because of excessive incorrect unlock attempts. */
        Undefined               /*!< Undefined - The state of the lock is unknown */
    };

    bool isEnabled() const;
    bool isUnlocking() const;
    LockState state() const;

    int automaticLocking() const;
    bool showNotifications() const;

    Q_INVOKABLE void unlock();
    Q_INVOKABLE void cancel();

signals:
    void enabledChanged();
    void unlockingChanged();
    void stateChanged();
    void automaticLockingChanged();
    void showNotificationsChanged();

    void locked();
    void unlocked();
    void unlockError();

private:
    inline void connected();

    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    LockState m_state;
    bool m_enabled;
    bool m_unlocking;
};

}

#endif
