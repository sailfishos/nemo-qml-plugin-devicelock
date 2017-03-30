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

#ifndef NEMODEVICELOCK_LOCKCODESETTINGS_H
#define NEMODEVICELOCK_LOCKCODESETTINGS_H

#include <nemo-devicelock/global.h>
#include <nemo-devicelock/private/connection.h>

#include <QDBusAbstractAdaptor>

namespace NemoDeviceLock
{

class SettingsWatcher;

class SecurityCodeSettings;
class SecurityCodeSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.client.SecurityCodeSettings")
public:
    explicit SecurityCodeSettingsAdaptor(SecurityCodeSettings *settings);

public slots:
    Q_NOREPLY void Changed(const QDBusVariant &authenticationToken);
    Q_NOREPLY void ChangeAborted();

    Q_NOREPLY void Cleared();
    Q_NOREPLY void ClearAborted();

private:
    SecurityCodeSettings * const m_settings;
};

class NEMODEVICELOCK_EXPORT SecurityCodeSettings : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(bool set READ isSet NOTIFY setChanged)
    Q_PROPERTY(bool mandatory READ isMandatory NOTIFY mandatoryChanged)
public:
    explicit SecurityCodeSettings(QObject *parent = nullptr);
    ~SecurityCodeSettings();

    bool isSet() const;
    bool isMandatory() const;

    Q_INVOKABLE void change(const QVariant &challengeCode);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void cancel();

signals:
    void setChanged();
    void mandatoryChanged();
    void changingChanged();
    void clearingChanged();

    void changed(const QVariant &authenticationToken);
    void changeAborted();

    void cleared();
    void clearAborted();

private:
    friend class SecurityCodeSettingsAdaptor;

    inline void connected();
    inline void handleChanged(const QVariant &authenticationToken);
    inline void handleChangeAborted();
    inline void handleCleared();
    inline void handleClearAborted();

    SecurityCodeSettingsAdaptor m_adaptor;
    QExplicitlySharedDataPointer<SettingsWatcher> m_settings;
    bool m_set;
    bool m_changing;
    bool m_clearing;
};

}

#endif
