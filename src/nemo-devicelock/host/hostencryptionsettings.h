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

#ifndef NEMODEVICELOCK_HOSTENCRYPTIONSETTINGS_H
#define NEMODEVICELOCK_HOSTENCRYPTIONSETTINGS_H

#include <nemo-devicelock/host/hostauthorization.h>

#include <QDBusVariant>

namespace NemoDeviceLock
{

class HostEncryptionSettings;
class HostEncryptionSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(bool Supported READ isSupported)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.EncryptionSettings")
public:
    explicit HostEncryptionSettingsAdaptor(HostEncryptionSettings *settings);

    bool isSupported() const;

public slots:
    void EncryptHome(const QDBusObjectPath &path, const QDBusVariant &authenticationToken);

private:
    HostEncryptionSettings * const m_settings;
};

class HostEncryptionSettings : public HostAuthorization
{
    Q_OBJECT
public:
    explicit HostEncryptionSettings(QObject *parent = nullptr);
    explicit HostEncryptionSettings(Authenticator::Methods allowedMethods, QObject *parent = nullptr);
    ~HostEncryptionSettings();

protected:
    virtual bool isSupported() const;

    virtual void encryptHome(const QString &client, const QVariant &authenticationToken);

private:
    friend class HostEncryptionSettingsAdaptor;

    HostEncryptionSettingsAdaptor m_adaptor;
};

}

#endif
