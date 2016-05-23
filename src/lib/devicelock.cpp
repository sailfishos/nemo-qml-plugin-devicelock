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
    , m_settings(SettingsWatcher::instance())
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &DeviceLock::automaticLockingChanged);
    connect(this, &DeviceLock::enabledChanged,
            this, &DeviceLock::automaticLockingChanged);
}

DeviceLock::~DeviceLock()
{
}

int DeviceLock::automaticLocking() const
{
    return isEnabled() ? m_settings->automaticLocking : -1;
}
