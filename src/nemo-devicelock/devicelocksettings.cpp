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

/*!
    \class NemoDeviceLock::DeviceLockSettings
    \brief The DeviceLockSettings class provides access to settings for device lock.
*/

/*!
    Constructs a new device lock settings instance which is a child of \a parent.
*/

DeviceLockSettings::DeviceLockSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/devicelock/settings"),
          QStringLiteral("org.nemomobile.devicelock.DeviceLock.Settings"))
    , m_authorization(m_localPath, path())
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

    m_connection->onConnected(this, [this] {
        connected();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys a device lock settings instance.
*/

DeviceLockSettings::~DeviceLockSettings()
{
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::authorization

    This property provides a means of acquiring authorization to change device lock settings.
*/

Authorization *DeviceLockSettings::authorization()
{
    return &m_authorization;
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::automaticLocking

    This property holds how long in minutes a device must be idle before it will automatically lock.
*/

int DeviceLockSettings::automaticLocking() const
{
    return m_settings->automaticLocking;
}

/*!
    Sets a new \a value for the automatic locking timeout.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setAutomaticLocking(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::automaticLockingKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::maximumAttempts

    This property holds the maximum number of consecutive times a user may enter an incorrect
    security code before they are locked out.
*/

int DeviceLockSettings::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

/*!
    Sets a new \a value for the maximum number of a security code entry attempts.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setMaximumAttempts(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::maximumAttemptsKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::peekingAllowed

    This property holds whether peeking from the lock screen is allowed when the device is locked.
*/

int DeviceLockSettings::peekingAllowed() const
{
    return m_settings->peekingAllowed;
}

/*!
    Sets a new \a value for the peeking allowed.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setPeekingAllowed(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::peekingAllowedKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::sideloadingAllowed

    This property holds whether sideloading of APK packages is allowed.
*/

int DeviceLockSettings::sideloadingAllowed() const
{
    return m_settings->sideloadingAllowed;
}

/*!
    Sets a new \a value for sideloading allowed.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setSideloadingAllowed(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::sideloadingAllowedKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::showNotifications

    This property holds whether notifications are shown on the lock screen when the device is locked.
*/

int DeviceLockSettings::showNotifications() const
{
    return m_settings->showNotifications;
}

/*!
    Sets a new \a value for show notifications.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setShowNotifications(const QVariant &authenticationToken, int value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::showNotificationsKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::inputIsKeyboard

    This property holds whether a full keyboard should be used for entering a security code.
*/

bool DeviceLockSettings::inputIsKeyboard() const
{
    return m_settings->inputIsKeyboard;
}

/*!
    Sets a new \a value for input is keyboard.

    The settings authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/

void DeviceLockSettings::setInputIsKeyboard(const QVariant &authenticationToken, bool value)
{
    changeSetting(authenticationToken, QString::fromUtf8(SettingsWatcher::inputIsKeyboardKey), value);
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::currentCodeIsDigitOnly

    This property holds whether the current code was entered using a numeric input field.
*/
bool DeviceLockSettings::currentCodeIsDigitOnly() const
{
    return m_settings->currentCodeIsDigitOnly;
}

/*!
    \property NemoDeviceLock::DeviceLockSettings::homeEncrypted

    This property holds whether the home folder has been encrypted.
*/

bool DeviceLockSettings::isHomeEncrypted() const
{
    return m_settings->isHomeEncrypted;
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
