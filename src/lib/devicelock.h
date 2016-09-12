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

#ifndef DEVICELOCK_H
#define DEVICELOCK_H

#include <clientauthorization.h>

class DeviceLock : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_ENUMS(LockState)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(LockState state READ state NOTIFY stateChanged)
    Q_PROPERTY(int automaticLocking READ automaticLocking NOTIFY automaticLockingChanged)
    Q_PROPERTY(Authorization *authorization READ authorization CONSTANT)
public:
    explicit DeviceLock(QObject *parent = nullptr);
    ~DeviceLock();

    enum LockState
    {
        Unlocked = 0,           /*!< Unlocked - The lock is unlocked */
        Locked,                 /*!< Locked - The lock is being used */
        Undefined               /*!< Undefined - The state of the lock is unknown */
    };

    bool isEnabled() const;
    LockState state() const;

    int automaticLocking() const;

    Authorization *authorization();

    Q_INVOKABLE void unlock(const QVariant &authenticationToken);

signals:
    void enabledChanged();
    void stateChanged();
    void automaticLockingChanged();

    void locked();
    void unlocked();
    void unlockError();

private:
    void connected();

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    LockState m_state;
    bool m_enabled;
};

#endif
