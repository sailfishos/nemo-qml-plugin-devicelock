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
#include <QEvent>
#include <QFile>
#include <QSettings>

#include <glib.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logging.h"

namespace NemoDeviceLock
{

QMetaEnum resolveMetaEnum(const QMetaObject *metaObject, const char *name)
{
    const int index = metaObject->indexOfEnumerator(name);
    return index != -1 ? metaObject->enumerator(index) : QMetaEnum();
}

int flagsFromString(const QMetaEnum &enumeration, const char *string)
{
    int flags = 0;
    for (const QByteArray &key : QByteArray(string).split(',')) {
        const int value = enumeration.keyToValue(key);
        if (value != -1) {
            flags |= value;
        }
    }
    return flags;
}

const char * const SettingsWatcher::automaticLockingKey = "automatic_locking";
const char * const SettingsWatcher::minimumLengthKey = "code_min_length";
const char * const SettingsWatcher::maximumLengthKey = "code_max_length";
const char * const SettingsWatcher::maximumAttemptsKey = "maximum_attempts";
const char * const SettingsWatcher::currentAttemptsKey = "current_attempts";
const char * const SettingsWatcher::peekingAllowedKey = "peeking_allowed";
const char * const SettingsWatcher::sideloadingAllowedKey = "sideloading_allowed";
const char * const SettingsWatcher::showNotificationsKey = "show_notification";
const char * const SettingsWatcher::inputIsKeyboardKey = "code_input_is_keyboard";
const char * const SettingsWatcher::currentIsDigitOnlyKey = "code_current_is_digit_only";
const char * const SettingsWatcher::isHomeEncryptedKey = "encrypt_home";

SettingsWatcher *SettingsWatcher::sharedInstance = nullptr;

SettingsWatcher::SettingsWatcher(QObject *parent)
    : QSocketNotifier(inotify_init(), Read, parent)
    , automaticLocking(10)
    , minimumLength(5)
    , maximumLength(42)
    , maximumAttempts(-1)
    , currentAttempts(0)
    , peekingAllowed(1)
    , sideloadingAllowed(-1)
    , showNotifications(1)
    , maximumAutomaticLocking(-1)
    , absoluteMaximumAttempts(-1)
    , supportedDeviceResetOptions(DeviceReset::Reboot)
    , codeGeneration(AuthenticationInput::NoCodeGeneration)
    , inputIsKeyboard(false)
    , currentCodeIsDigitOnly(true)
    , isHomeEncrypted(false)
    , codeIsMandatory(false)
    , m_settingsPath(QStringLiteral("/usr/share/lipstick/devicelock/devicelock_settings.conf"))
    , m_watch(-1)
{
    Q_ASSERT(!sharedInstance);
    sharedInstance = this;

    m_watch = inotify_add_watch(
                socket(),
                "/usr/share/lipstick/devicelock",
                IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM | IN_CLOSE_WRITE | IN_DELETE);

    reloadSettings();
}

SettingsWatcher::~SettingsWatcher()
{
    close(socket());

    sharedInstance = nullptr;
}

SettingsWatcher *SettingsWatcher::instance()
{
    if (sharedInstance)
        return sharedInstance;

    return sharedInstance ? sharedInstance : new SettingsWatcher;
}

bool SettingsWatcher::event(QEvent *event)
{
    if (event->type() == QEvent::SockAct) {
        int bufferSize = 0;
        ioctl(socket(), FIONREAD, (char *) &bufferSize);
        QVarLengthArray<char, 4096> buffer(bufferSize);

        bufferSize = read(socket(), buffer.data(), bufferSize);
        char *at = buffer.data();
        char * const end = at + bufferSize;

        struct inotify_event *pevent = 0;
        for (;at < end; at += sizeof(inotify_event) + pevent->len) {
            pevent = reinterpret_cast<inotify_event *>(at);

            if (pevent->wd == m_watch
                    && pevent->len > 0
                    && QLatin1String(pevent->name) == QLatin1String("devicelock_settings.conf")) {
                reloadSettings();
            }
        }

        return true;
    } else {
        return QSocketNotifier::event(event);
    }
}

template <typename T> T readConfigValue(GKeyFile *config, const char *group, const char *key, T defaultValue)
{
    GError *error = nullptr;
    gchar *string = g_key_file_get_string(config, group, key, &error);
    if (error) {
        if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND
                && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            qCWarning(devicelock) << "Error reading" << group << key << error->message;
        }
        g_error_free(error);

        return defaultValue;
    } else {
        const T value = settingsValueFromString<T>(string);

        g_free(string);

        return value;
    }
}

template <> int readConfigValue<int>(GKeyFile *config, const char *group, const char *key, int defaultValue)
{
    GError *error = nullptr;
    const int value = g_key_file_get_integer(config, group, key, &error);
    if (error ) {
        if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND
                && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            qCWarning(devicelock) << "Error reading" << group << key << error->message;
        }
        g_error_free(error);

        return defaultValue;
    } else {
        return value;
    }
}

template <> bool readConfigValue<bool>(
        GKeyFile *config, const char *group, const char *key, bool defaultValue)
{
    GError *error = nullptr;
    const gboolean value = g_key_file_get_boolean(config, group, key, &error);
    if (error) {
        if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND
                && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
            qCWarning(devicelock) << "Error reading" << group << key << error->message;
        }
        g_error_free(error);

        return defaultValue;
    } else {
        return value;
    }
}

template <typename T>
static void read(
        GKeyFile *settings,
        SettingsWatcher *watcher,
        const char *group,
        const char *key,
        T defaultValue,
        T *member,
        void (SettingsWatcher::*changed)() = nullptr)
{
    const T value = readConfigValue<T>(settings, group, key, defaultValue);

    if (*member != value) {
        *member = value;
        if (changed) {
            emit (watcher->*changed)();
        }
    }
}

template <typename T>
static void read(
        GKeyFile *settings,
        SettingsWatcher *watcher,
        const char *key,
        T defaultValue,
        T *member,
        void (SettingsWatcher::*changed)() = nullptr)
{
    read(settings,
                watcher,
                "desktop",
                (QByteArrayLiteral("nemo\\devicelock\\") + key).constData(),
                defaultValue,
                member,
                changed);
}

void SettingsWatcher::reloadSettings()
{
    GKeyFile * const settings = g_key_file_new();
    g_key_file_load_from_file(settings, m_settingsPath.toUtf8().constData(), G_KEY_FILE_NONE, 0);

    read(settings, this, automaticLockingKey, 5, &automaticLocking, &SettingsWatcher::automaticLockingChanged);
    read(settings, this, minimumLengthKey, 5, &minimumLength, &SettingsWatcher::minimumLengthChanged);
    read(settings, this, maximumLengthKey, 42, &maximumLength, &SettingsWatcher::maximumLengthChanged);
    read(settings, this, maximumAttemptsKey, -1, &maximumAttempts, &SettingsWatcher::maximumAttemptsChanged);
    read(settings, this, currentAttemptsKey, 0, &currentAttempts, &SettingsWatcher::currentAttemptsChanged);
    read(settings, this, peekingAllowedKey, 1, &peekingAllowed, &SettingsWatcher::peekingAllowedChanged);
    read(settings, this, sideloadingAllowedKey, -1, &sideloadingAllowed, &SettingsWatcher::sideloadingAllowedChanged);
    read(settings, this, showNotificationsKey, 1, &showNotifications, &SettingsWatcher::showNotificationsChanged);
    read(settings, this, inputIsKeyboardKey, false, &inputIsKeyboard, &SettingsWatcher::inputIsKeyboardChanged);
    read(settings, this, currentIsDigitOnlyKey, true, &currentCodeIsDigitOnly, &SettingsWatcher::currentCodeIsDigitOnlyChanged);
    read(settings, this, isHomeEncryptedKey, false, &isHomeEncrypted);

    read(settings, this, "maximum_automatic_locking", -1, &maximumAutomaticLocking, &SettingsWatcher::maximumAutomaticLockingChanged);
    read(settings, this, "absolute_maximum_attempts", -1, &absoluteMaximumAttempts, &SettingsWatcher::absoluteMaximumAttemptsChanged);
    read(settings, this, "supported_device_reset_options", DeviceReset::Options(DeviceReset::Reboot), &supportedDeviceResetOptions, &SettingsWatcher::supportedDeviceResetOptionsChanged);
    read(settings, this, "code_is_mandatory", false, &codeIsMandatory, &SettingsWatcher::codeIsMandatoryChanged);
    read(settings, this, "code_generation", AuthenticationInput::NoCodeGeneration, &codeGeneration, &SettingsWatcher::codeGenerationChanged);

    g_key_file_free(settings);
}

}
