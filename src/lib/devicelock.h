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

#include <QObject>

#include <QSharedDataPointer>
#include <QVariant>

class Authorization;
class SettingsWatcher;

class DeviceLock : public QObject
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

    virtual bool isEnabled() const = 0;
    virtual LockState state() const = 0;

    int automaticLocking() const;

    virtual Authorization *authorization() = 0;

    Q_INVOKABLE virtual void unlock(const QVariant &authenticationToken) = 0;

signals:
    void enabledChanged();
    void stateChanged();
    void automaticLockingChanged();

    void locked();
    void unlocked();
    void unlockError();

private:
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
};

#endif
