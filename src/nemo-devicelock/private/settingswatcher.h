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

#ifndef NEMODEVICELOCK_SETTINGSWATCHER_H
#define NEMODEVICELOCK_SETTINGSWATCHER_H

#include <nemo-devicelock/authenticationinput.h>
#include <nemo-devicelock/devicereset.h>

#include <QMetaEnum>
#include <QSharedData>
#include <QSocketNotifier>

namespace NemoDeviceLock
{

const QString alphanumEncryptionCodeSetFile("/usr/share/lipstick/devicelock/.alphanumeric-encryption-set_%1.enc");

QMetaEnum NEMODEVICELOCK_EXPORT resolveMetaEnum(const QMetaObject *metaObject, const char *name);
template <typename Enum> inline QMetaEnum resolveMetaEnum();

int NEMODEVICELOCK_EXPORT flagsFromString(const QMetaEnum &enumeration, const char *string);
template <typename Enum> inline QFlags<Enum> flagsFromString(const char *string) {
    return QFlags<Enum>(flagsFromString(resolveMetaEnum<Enum>(), string)); }

template <typename T> inline T settingsValueFromString(const char *string);

template <> inline QMetaEnum resolveMetaEnum<DeviceReset::Option>()  {
    return resolveMetaEnum(&DeviceReset::staticMetaObject, "Option"); }
template <> inline DeviceReset::Options settingsValueFromString<DeviceReset::Options>(const char *string) {
    return flagsFromString<DeviceReset::Option>(string); }

template <> inline QMetaEnum resolveMetaEnum<AuthenticationInput::CodeGeneration>()  {
    return resolveMetaEnum(&AuthenticationInput::staticMetaObject, "CodeGeneration"); }
template <> inline AuthenticationInput::CodeGeneration settingsValueFromString<AuthenticationInput::CodeGeneration>(const char *string) {
    return AuthenticationInput::CodeGeneration(flagsFromString(resolveMetaEnum<AuthenticationInput::CodeGeneration>(), string)); }

class NEMODEVICELOCK_EXPORT SettingsWatcher : public QSocketNotifier, public QSharedData
{
    Q_OBJECT
public:
    ~SettingsWatcher();

    static SettingsWatcher *instance();

    int automaticLocking;
    int currentLength;
    int minimumLength;
    int maximumLength;
    int maximumAttempts;
    int currentAttempts;
    int peekingAllowed;
    int sideloadingAllowed;
    int showNotifications;
    int maximumAutomaticLocking;
    int absoluteMaximumAttempts;
    DeviceReset::Options supportedDeviceResetOptions;
    AuthenticationInput::CodeGeneration codeGeneration;
    bool inputIsKeyboard;
    bool currentCodeIsDigitOnly;
    bool isHomeEncrypted;
    bool codeIsMandatory;
    bool alphanumericEncryptionSet;

    static const char * const automaticLockingKey;
    static const char * const currentLengthKey;
    static const char * const minimumLengthKey;
    static const char * const maximumLengthKey;
    static const char * const maximumAttemptsKey;
    static const char * const currentAttemptsKey;
    static const char * const peekingAllowedKey;
    static const char * const sideloadingAllowedKey;
    static const char * const showNotificationsKey;
    static const char * const inputIsKeyboardKey;
    static const char * const currentIsDigitOnlyKey;
    static const char * const isHomeEncryptedKey;

    bool event(QEvent *event);

signals:
    void automaticLockingChanged();
    void maximumAttemptsChanged();
    void currentAttemptsChanged();
    void currentLengthChanged();
    void minimumLengthChanged();
    void maximumLengthChanged();
    void peekingAllowedChanged();
    void sideloadingAllowedChanged();
    void showNotificationsChanged();
    void absoluteMaximumAttemptsChanged();
    void maximumAutomaticLockingChanged();
    void supportedDeviceResetOptionsChanged();
    void inputIsKeyboardChanged();
    void currentCodeIsDigitOnlyChanged();
    void codeIsMandatoryChanged();
    void codeGenerationChanged();
    void alphanumericEncryptionSetChanged();

private:
    explicit SettingsWatcher(QObject *parent = nullptr);

    void reloadSettings();
    void readAlphanumericEncryptionSet();
    QString getAlphanumEncryptionSetFile();

    QString m_settingsPath;
    QString m_alphanumEncryptionCodeSetFile;
    int m_watch;

    static SettingsWatcher *sharedInstance;
};

}

#endif
