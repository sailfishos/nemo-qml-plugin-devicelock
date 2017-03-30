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

#ifndef NEMODEVICELOCK_DEVICELOCKSETTINGS_H
#define NEMODEVICELOCK_DEVICELOCKSETTINGS_H

#include <nemo-devicelock/private/clientauthorization.h>

namespace NemoDeviceLock
{

class SettingsWatcher;

class NEMODEVICELOCK_EXPORT DeviceLockSettings : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(NemoDeviceLock::Authorization *authorization READ authorization CONSTANT)
    Q_PROPERTY(int automaticLocking READ automaticLocking NOTIFY automaticLockingChanged)
    Q_PROPERTY(int maximumAttempts READ maximumAttempts NOTIFY maximumAttemptsChanged)
    Q_PROPERTY(bool peekingAllowed READ peekingAllowed NOTIFY peekingAllowedChanged)
    Q_PROPERTY(bool sideloadingAllowed READ sideloadingAllowed NOTIFY sideloadingAllowedChanged)
    Q_PROPERTY(bool showNotifications READ showNotifications NOTIFY showNotificationsChanged)
    Q_PROPERTY(bool inputIsKeyboard READ inputIsKeyboard NOTIFY inputIsKeyboardChanged)
    Q_PROPERTY(bool currentCodeIsDigitOnly READ currentCodeIsDigitOnly NOTIFY currentCodeIsDigitOnlyChanged)
    Q_PROPERTY(bool homeEncrypted READ isHomeEncrypted CONSTANT)
    Q_PROPERTY(int maximumAutomaticLocking READ maximumAutomaticLocking NOTIFY maximumAutomaticLockingChanged)
    Q_PROPERTY(int absoluteMaximumAttempts READ absoluteMaximumAttempts NOTIFY absoluteMaximumAttemptsChanged)
public:
    explicit DeviceLockSettings(QObject *parent = nullptr);
    ~DeviceLockSettings();

    Authorization *authorization();

    int automaticLocking() const;
    Q_INVOKABLE void setAutomaticLocking(const QVariant &authenticationToken, int value);

    int maximumAttempts() const;
    Q_INVOKABLE void setMaximumAttempts(const QVariant &authenticationToken, int value);

    bool peekingAllowed() const;
    Q_INVOKABLE void setPeekingAllowed(const QVariant &authenticationToken, bool value);

    bool sideloadingAllowed() const;
    Q_INVOKABLE void setSideloadingAllowed(const QVariant &authenticationToken, bool value);

    bool showNotifications() const;
    Q_INVOKABLE void setShowNotifications(const QVariant &authenticationToken, bool value);

    bool inputIsKeyboard() const;
    Q_INVOKABLE void setInputIsKeyboard(const QVariant &authenticationToken, bool value);

    bool currentCodeIsDigitOnly() const;

    bool isHomeEncrypted() const;

    int maximumAutomaticLocking() const;
    int absoluteMaximumAttempts() const;

signals:
    void automaticLockingChanged();
    void maximumAttemptsChanged();
    void peekingAllowedChanged();
    void sideloadingAllowedChanged();
    void showNotificationsChanged();
    void inputIsKeyboardChanged();
    void currentCodeIsDigitOnlyChanged();
    void maximumAutomaticLockingChanged();
    void absoluteMaximumAttemptsChanged();

private:
    inline void changeSetting(
            const QVariant &authenticationToken, const QString &key, const QVariant &value);
    inline void connected();

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
};

}

#endif
