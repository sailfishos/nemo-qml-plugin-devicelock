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

#ifndef LOCKCODESETTINGS_H
#define LOCKCODESETTINGS_H

#include <connection.h>

namespace NemoDeviceLock
{

class SettingsWatcher;

class LockCodeSettings : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(bool set READ isSet NOTIFY setChanged)
    Q_PROPERTY(int minimumLength READ minimumLength CONSTANT)
    Q_PROPERTY(int maximumLength READ maximumLength CONSTANT)
public:
    explicit LockCodeSettings(QObject *parent = nullptr);
    ~LockCodeSettings();

    bool isSet() const;
    int minimumLength() const;
    int maximumLength() const;

    Q_INVOKABLE void change(const QString &oldCode, const QString &newCode);
    Q_INVOKABLE void clear(const QString &currentCode);

signals:
    void setChanged();
    void changed();
    void cleared();
    void changeError();
    void clearError();

private:
    void connected();

    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;

    bool m_set;
};

}

#endif
