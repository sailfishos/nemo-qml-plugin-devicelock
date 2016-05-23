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

#ifndef DEVICELOCKSETTINGS_H
#define DEVICELOCKSETTINGS_H

#include <QObject>

#include <QSharedDataPointer>
#include <QVariant>

class Authorization;
class SettingsWatcher;

class DeviceLockSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Authorization *authorization READ authorization CONSTANT)
    Q_PROPERTY(int automaticLocking READ automaticLocking NOTIFY automaticLockingChanged)
    Q_PROPERTY(int maximumAttempts READ maximumAttempts NOTIFY maximumAttemptsChanged)
    Q_PROPERTY(int peekingAllowed READ peekingAllowed NOTIFY peekingAllowedChanged)
    Q_PROPERTY(int sideloadingAllowed READ sideloadingAllowed NOTIFY sideloadingAllowedChanged)
    Q_PROPERTY(int showNotifications READ showNotifications NOTIFY showNotificationsChanged)
    Q_PROPERTY(bool inputIsKeyboard READ inputIsKeyboard NOTIFY inputIsKeyboardChanged)
    Q_PROPERTY(bool currentCodeIsDigitOnly READ currentCodeIsDigitOnly NOTIFY currentCodeIsDigitOnlyChanged)
public:
    explicit DeviceLockSettings(QObject *parent = nullptr);
    ~DeviceLockSettings();

    virtual Authorization *authorization() = 0;

    int automaticLocking() const;
    Q_INVOKABLE void setAutomaticLocking(const QVariant &authenticationToken, int value);

    int maximumAttempts() const;
    Q_INVOKABLE void setMaximumAttempts(const QVariant &authenticationToken, int value);

    int peekingAllowed() const;
    Q_INVOKABLE void setPeekingAllowed(const QVariant &authenticationToken, int value);

    int sideloadingAllowed() const;
    Q_INVOKABLE void setSideloadingAllowed(const QVariant &authenticationToken, int value);

    int showNotifications() const;
    Q_INVOKABLE void setShowNotifications(const QVariant &authenticationToken,int value);

    bool inputIsKeyboard() const;
    Q_INVOKABLE void setInputIsKeyboard(const QVariant &authenticationToken, bool value);

    bool currentCodeIsDigitOnly() const;

signals:
    void challengeCodeChanged();

    void automaticLockingChanged();
    void maximumAttemptsChanged();
    void peekingAllowedChanged();
    void sideloadingAllowedChanged();
    void showNotificationsChanged();
    void inputIsKeyboardChanged();
    void currentCodeIsDigitOnlyChanged();

protected:

    virtual void changeSetting(const QVariant &authenticationToken, const QString &key, const QVariant &value) = 0;

private:
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
};

#endif
