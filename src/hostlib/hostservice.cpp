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

#include "hostservice.h"

#include "hostauthenticator.h"
#include "hostdevicelock.h"
#include "hostdevicelocksettings.h"
#include "hostdevicereset.h"
#include "hostencryptionsettings.h"
#include "hostfingerprintsettings.h"
#include "hostlockcodesettings.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDir>

#include <QDebug>

#include <dbus/dbus.h>
#include <systemd/sd-daemon.h>

template <typename T, int N> constexpr int lengthOf(const T(&)[N]) { return N; }

class ConnectionMonitor : public QObject
{
    Q_OBJECT
public:
    ConnectionMonitor(HostService *service, const QString &connectionName)
        : QObject(service)
        , m_service(service)
        , m_connectionName(connectionName)
    {
    }

public slots:
    void disconnected()
    {
        deleteLater();

        for (int i = 0; i < lengthOf(m_service->m_objects); ++i) {
            m_service->m_objects[i]->clientDisconnected(m_connectionName);
        }
    }

private:
    HostService * const m_service;
    const QString m_connectionName;
};

HostService::HostService(
        HostAuthenticator *authenticator,
        HostDeviceLock *deviceLock,
        HostDeviceLockSettings *deviceLockSettings,
        HostDeviceReset *deviceReset,
        HostEncryptionSettings *encryptionSettings,
        HostFingerprintSensor *fingerprintSensor,
        HostFingerprintSettings *fingerprintSettings,
        HostLockCodeSettings *lockCodeSettings,
        QObject *parent)
    : QDBusServer(QStringLiteral("unix:path=/run/org-nemomobile-devicelock.socket"), parent)
    , m_authenticator(authenticator)
    , m_deviceLock(deviceLock)
    , m_deviceLockSettings(deviceLockSettings)
    , m_deviceReset(deviceReset)
    , m_encryptionSettings(encryptionSettings)
    , m_fingerprintSensor(fingerprintSensor)
    , m_fingerprintSettings(fingerprintSettings)
    , m_lockCodeSettings(lockCodeSettings)
{
    connect(this, &QDBusServer::newConnection, this, &HostService::connectionReady);

    qDBusRegisterMetaType<Fingerprint>();
    qDBusRegisterMetaType<QVector<Fingerprint>>();

    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.nemomobile.devicelock"))) {
        qDebug() << "Failed to register service org.nemomobile.devicelock" << QDBusConnection::systemBus().lastError();
    }

    sd_notify(0, "READY=1");
}

HostService::~HostService()
{
}

static void registerObject(QDBusConnection &connection, const QString &path, QObject *object)
{
    if (!connection.registerObject(path, object)) {
        qWarning() << "Failed to register object on path" << path;
    }
}

static dbus_bool_t authenticateUser(DBusConnection *, unsigned long, void *)
{
    return TRUE;
}

void HostService::connectionReady(const QDBusConnection &newConnection)
{
    QDBusConnection connection(newConnection);

    const auto monitor = new ConnectionMonitor(this, connection.name());

    if (!connection.connect(
                QString(),
                QStringLiteral("/org/freedesktop/DBus/Local"),
                QStringLiteral("org.freedesktop.DBus.Local"),
                QStringLiteral("Disconnected"),
                monitor,
                SLOT(disconnected()))) {
        delete monitor;

        qWarning() << "Failed to connect to disconnect signal";
    }

    const auto connectionName = newConnection.name();

    for (int i = 0; i < lengthOf(m_objects); ++i) {
        const auto object = m_objects[i];

        registerObject(connection, object->path(), object);
        object->clientConnected(connectionName);
    }

    auto *internalConnection = static_cast<DBusConnection*>(connection.internalPointer());

    dbus_connection_set_unix_user_function(internalConnection, authenticateUser, nullptr,  nullptr);
}

#include "hostservice.moc"
