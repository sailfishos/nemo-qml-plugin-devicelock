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

#include "settingswatcher.h"

#include <QDebug>
#include <QFile>
#include <QSettings>

#include "logging.h"

namespace NemoDeviceLock
{

const char * const SettingsWatcher::automaticLockingKey = "/desktop/nemo/devicelock/automatic_locking";
const char * const SettingsWatcher::minimumLengthKey = "/desktop/nemo/devicelock/code_min_length";
const char * const SettingsWatcher::maximumLengthKey = "/desktop/nemo/devicelock/code_max_length";
const char * const SettingsWatcher::maximumAttemptsKey = "/desktop/nemo/devicelock/maximum_attempts";
const char * const SettingsWatcher::currentAttemptsKey = "/desktop/nemo/devicelock/current_attempts";
const char * const SettingsWatcher::peekingAllowedKey = "/desktop/nemo/devicelock/peeking_allowed";
const char * const SettingsWatcher::sideloadingAllowedKey = "/desktop/nemo/devicelock/sideloading_allowed";
const char * const SettingsWatcher::showNotificationsKey = "/desktop/nemo/devicelock/show_notification";
const char * const SettingsWatcher::inputIsKeyboardKey = "/desktop/nemo/devicelock/code_input_is_keyboard";
const char * const SettingsWatcher::currentIsDigitOnlyKey = "/desktop/nemo/devicelock/code_current_is_digit_only";
const char * const SettingsWatcher::isHomeEncryptedKey = "/desktop/nemo/devicelock/encrypt_home";

SettingsWatcher *SettingsWatcher::sharedInstance = nullptr;

SettingsWatcher::SettingsWatcher(QObject *parent)
    : QObject(parent)
    , automaticLocking(10)
    , minimumLength(5)
    , maximumLength(42)
    , maximumAttempts(-1)
    , currentAttempts(0)
    , peekingAllowed(1)
    , sideloadingAllowed(-1)
    , showNotifications(1)
    , inputIsKeyboard(false)
    , currentCodeIsDigitOnly(true)
    , isHomeEncrypted(false)
    , m_settingsPath(QStringLiteral("/usr/share/lipstick/devicelock/devicelock_settings.conf"))
{
    Q_ASSERT(!sharedInstance);
    sharedInstance = this;

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &SettingsWatcher::reloadSettings);

    if (QFile::exists(m_settingsPath) && m_watcher.addPath(m_settingsPath)) {
        reloadSettings();
    } else {
        qCWarning(devicelock, "Unable to follow device lock configuration file changes");
    }
}

SettingsWatcher::~SettingsWatcher()
{
    sharedInstance = nullptr;
}

SettingsWatcher *SettingsWatcher::instance()
{
    if (sharedInstance)
        return sharedInstance;

    return sharedInstance ? sharedInstance : new SettingsWatcher;
}

template <typename T>
static void read(
        const QSettings &settings,
        SettingsWatcher *watcher,
        const char *key,
        T defaultValue,
        T (SettingsWatcher::*member),
        void (SettingsWatcher::*changed)() = nullptr)
{
    T value = settings.value(QString::fromUtf8(key), QVariant(defaultValue)).value<T>();

    if (watcher->*member != value) {
        watcher->*member = value;
        if (changed) {
            emit (watcher->*changed)();
        }
    }
}

void SettingsWatcher::reloadSettings()
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);

    read(settings, this, automaticLockingKey, 10, &SettingsWatcher::automaticLocking, &SettingsWatcher::automaticLockingChanged);
    read(settings, this, minimumLengthKey, 5, &SettingsWatcher::minimumLength, &SettingsWatcher::minimumLengthChanged);
    read(settings, this, maximumLengthKey, 42, &SettingsWatcher::maximumLength, &SettingsWatcher::maximumLengthChanged);
    read(settings, this, maximumAttemptsKey, -1, &SettingsWatcher::maximumAttempts, &SettingsWatcher::maximumAttemptsChanged);
    read(settings, this, currentAttemptsKey, 0, &SettingsWatcher::currentAttempts, &SettingsWatcher::currentAttemptsChanged);
    read(settings, this, peekingAllowedKey, 1, &SettingsWatcher::peekingAllowed, &SettingsWatcher::peekingAllowedChanged);
    read(settings, this, sideloadingAllowedKey, -1, &SettingsWatcher::sideloadingAllowed, &SettingsWatcher::sideloadingAllowedChanged);
    read(settings, this, showNotificationsKey, 1, &SettingsWatcher::showNotifications, &SettingsWatcher::showNotificationsChanged);
    read(settings, this, inputIsKeyboardKey, false, &SettingsWatcher::inputIsKeyboard, &SettingsWatcher::inputIsKeyboardChanged);
    read(settings, this, currentIsDigitOnlyKey, true, &SettingsWatcher::currentCodeIsDigitOnly, &SettingsWatcher::currentCodeIsDigitOnlyChanged);
    read(settings, this, isHomeEncryptedKey, false, &SettingsWatcher::isHomeEncrypted);
}

}
