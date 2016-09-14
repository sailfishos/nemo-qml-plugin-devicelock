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

#include <QQmlExtensionPlugin>

#include <authenticator.h>
#include <devicelock.h>
#include <devicelocksettings.h>
#include <devicereset.h>
#include <encryptionsettings.h>
#include <fingerprintsettings.h>
#include <lockcodesettings.h>

#include <qqml.h>
#include <QQmlEngine>

#include <QDBusMetaType>

static QObject *createDeviceLock(QQmlEngine *, QJSEngine *)
{
    return new NemoDeviceLock::DeviceLock;
}

class Q_DECL_EXPORT NemoDeviceLockPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.nemomobile.devicelock")
public:
    void initializeEngine(QQmlEngine *, const char *) override
    {
    }

    void registerTypes(const char *uri) override
    {
        qDBusRegisterMetaType<NemoDeviceLock::Fingerprint>();
        qDBusRegisterMetaType<QVector<NemoDeviceLock::Fingerprint>>();

        qmlRegisterType<NemoDeviceLock::FingerprintModel>();

        qmlRegisterSingletonType<NemoDeviceLock::DeviceLock>(uri, 1, 0, "DeviceLock", createDeviceLock);

        qmlRegisterType<NemoDeviceLock::Authenticator>(uri, 1, 0, "Authenticator");
        qmlRegisterType<NemoDeviceLock::DeviceLockSettings>(uri, 1, 0, "DeviceLockSettings");
        qmlRegisterType<NemoDeviceLock::DeviceReset>(uri, 1, 0, "DeviceReset");
        qmlRegisterType<NemoDeviceLock::EncryptionSettings>(uri, 1, 0, "EncryptionSettings");
        qmlRegisterType<NemoDeviceLock::FingerprintSettings>(uri, 1, 0, "FingerprintSettings");
        qmlRegisterType<NemoDeviceLock::LockCodeSettings>(uri, 1, 0, "LockCodeSettings");

        qmlRegisterUncreatableType<NemoDeviceLock::Authorization>(uri, 1, 0, "Authorization", QString());
    }
};

#include "plugin.moc"
