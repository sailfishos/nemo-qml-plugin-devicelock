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

#ifndef SETTINGSWATCHER_H
#define SETTINGSWATCHER_H

#include <QSharedData>
#include <QSocketNotifier>

class SettingsWatcher : public QSocketNotifier, public QSharedData
{
    Q_OBJECT
public:
    ~SettingsWatcher();

    static SettingsWatcher *instance();

    int automaticLocking;
    int minimumLength;
    int maximumLength;
    int maximumAttempts;
    int peekingAllowed;
    int sideloadingAllowed;
    int showNotifications;
    bool inputIsKeyboard;
    bool currentCodeIsDigitOnly;

    static const char * const automaticLockingKey;
    static const char * const minimumLengthKey;
    static const char * const maximumLengthKey;
    static const char * const maximumAttemptsKey;
    static const char * const peekingAllowedKey;
    static const char * const sideloadingAllowedKey;
    static const char * const showNotificationsKey;
    static const char * const inputIsKeyboardKey;
    static const char * const currentIsDigitOnlyKey;

    bool event(QEvent *event);

signals:
    void automaticLockingChanged();
    void maximumAttemptsChanged();
    void minimumLengthChanged();
    void maximumLengthChanged();
    void peekingAllowedChanged();
    void sideloadingAllowedChanged();
    void showNotificationsChanged();
    void inputIsKeyboardChanged();
    void currentCodeIsDigitOnlyChanged();

private:
    explicit SettingsWatcher(QObject *parent = nullptr);

    void watchForSettingsFile();
    void watchSettingsFile();
    void reloadSettings();

    QString m_settingsPath;
    int m_watch;

    static SettingsWatcher *sharedInstance;


};

#endif
