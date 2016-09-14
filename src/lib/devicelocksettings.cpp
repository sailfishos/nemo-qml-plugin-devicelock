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

#include "devicelocksettings.h"

#include "settingswatcher.h"

namespace NemoDeviceLock
{

DeviceLockSettings::DeviceLockSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/devicelock/settings"),
          QStringLiteral("org.nemomobile.devicelock.DeviceLock.Settings"))
    , m_authorization(m_localPath, m_remotePath)
    , m_authorizationAdaptor(&m_authorization, this)
    , m_settings(SettingsWatcher::instance())
{
    connect(m_settings.data(), &SettingsWatcher::automaticLockingChanged,
            this, &DeviceLockSettings::automaticLockingChanged);
    connect(m_settings.data(), &SettingsWatcher::maximumAttemptsChanged,
            this, &DeviceLockSettings::maximumAttemptsChanged);
    connect(m_settings.data(), &SettingsWatcher::peekingAllowedChanged,
            this, &DeviceLockSettings::peekingAllowedChanged);
    connect(m_settings.data(), &SettingsWatcher::sideloadingAllowedChanged,
            this, &DeviceLockSettings::sideloadingAllowedChanged);
    connect(m_settings.data(), &SettingsWatcher::showNotificationsChanged,
            this, &DeviceLockSettings::showNotificationsChanged);
    connect(m_settings.data(), &SettingsWatcher::inputIsKeyboardChanged,
            this, &DeviceLockSettings::inputIsKeyboardChanged);
    connect(m_settings.data(), &SettingsWatcher::currentCodeIsDigitOnlyChanged,
            this, &DeviceLockSettings::currentCodeIsDigitOnlyChanged);

    connect(m_connection.data(), &Connection::connected, this, &DeviceLockSettings::connected);

    if (m_connection->isConnected()) {
        connected();
    }
}

DeviceLockSettings::~DeviceLockSettings()
{
}

Authorization *DeviceLockSettings::authorization()
{
    return &m_authorization;
}

int DeviceLockSettings::automaticLocking() const
{
    return m_settings->automaticLocking;
}

void DeviceLockSettings::setAutomaticLocking(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::automaticLockingKey), value);
}

int DeviceLockSettings::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

void DeviceLockSettings::setMaximumAttempts(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::maximumAttemptsKey), value);
}

int DeviceLockSettings::peekingAllowed() const
{
    return m_settings->peekingAllowed;
}

void DeviceLockSettings::setPeekingAllowed(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::peekingAllowedKey), value);
}

int DeviceLockSettings::sideloadingAllowed() const
{
    return m_settings->sideloadingAllowed;
}

void DeviceLockSettings::setSideloadingAllowed(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::sideloadingAllowedKey), value);
}

int DeviceLockSettings::showNotifications() const
{
    return m_settings->showNotifications;
}

void DeviceLockSettings::setShowNotifications(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::showNotificationsKey), value);
}

bool DeviceLockSettings::inputIsKeyboard() const
{
    return m_settings->inputIsKeyboard;
}

void DeviceLockSettings::setInputIsKeyboard(const QVariant &authenticationToken, bool value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::inputIsKeyboardKey), value);
}

bool DeviceLockSettings::currentCodeIsDigitOnly() const
{
    return m_settings->currentCodeIsDigitOnly;
}

void DeviceLockSettings::changeSetting(
        const QVariant &authenticationToken, const QString &key, const QVariant &value)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        call(QStringLiteral("ChangeSetting"), m_localPath, authenticationToken, key, value);
    }
}

void DeviceLockSettings::connected()
{
    registerObject();
}

}
