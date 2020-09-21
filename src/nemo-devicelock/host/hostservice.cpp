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
#include "hostfingerprintsensor.h"
#include "hostfingerprintsettings.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDir>
#include <QThread>

#include <dbus/dbus.h>
#include <systemd/sd-daemon.h>

namespace NemoDeviceLock
{

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

        for (const auto object : m_service->m_objects) {
            object->clientDisconnected(m_connectionName);
        }

        QDBusConnection::disconnectFromPeer(m_connectionName);
    }

private:
    HostService * const m_service;
    const QString m_connectionName;
};

HostService::HostService(const QVector<HostObject *> objects, QObject *parent)
    : QDBusServer(HostService::socketAddress(), parent)
    , m_objects(objects)
{
    setAnonymousAuthenticationAllowed(true);

    connect(this, &QDBusServer::newConnection, this, &HostService::connectionReady);

    qDBusRegisterMetaType<NemoDeviceLock::Fingerprint>();
    qDBusRegisterMetaType<QVector<NemoDeviceLock::Fingerprint>>();

    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.nemomobile.devicelock"))) {
        qCWarning(daemon, "Failed to register service org.nemomobile.devicelock. %s",
                    qPrintable(QDBusConnection::systemBus().lastError().message()));
    }

    if (isConnected()) {
        sd_notify(0, "READY=1");
    } else {
        qCCritical(daemon, "Failed to start device lock DBus server: %s",
                    qPrintable(lastError().message()));
    }
}

HostService::HostService(
        HostAuthenticator *authenticator,
        HostDeviceLock *deviceLock,
        HostDeviceLockSettings *deviceLockSettings,
        HostDeviceReset *deviceReset,
        HostEncryptionSettings *encryptionSettings,
        HostFingerprintSensor *fingerprintSensor,
        HostFingerprintSettings *fingerprintSettings,
        QObject *parent)
    : HostService({
            authenticator,
            deviceLock,
            deviceLockSettings,
            deviceReset,
            encryptionSettings,
            fingerprintSensor,
            fingerprintSettings }
        , parent)
{
}

HostService::~HostService()
{
}

static void registerObject(QDBusConnection &connection, const QString &path, QObject *object)
{
    if (!connection.registerObject(path, object)) {
        qCWarning(daemon, "Failed to register object on path %s", qPrintable(path));
    }
}

void HostService::connectionReady(const QDBusConnection &newConnection)
{
    qCDebug(daemon, "New connection %s", qPrintable(newConnection.name()));

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

        qCWarning(daemon, "Failed to connect to disconnect signal");
    }

    // The PID and UID of the connecting process can't be acquired until after the connection
    // is authenticated which is done concurrently to the invokation of this slot.  Block until
    // the connection is authenticated or disconnected before trying to decide which services
    // a process should have access to.
    auto internalConnection = static_cast<DBusConnection *>(connection.internalPointer());
    while (dbus_connection_get_is_connected(internalConnection)
           && !dbus_connection_get_is_authenticated(internalConnection)) {
        QThread::usleep(100);
    }

    const auto connectionName = connection.name();

    for (const auto object : m_objects) {
        if (object->authorizeConnection(connection)) {
            registerObject(connection, object->path(), object);
            object->clientConnected(connectionName);
        }
    }
}

QString HostService::socketAddress()
{
    // Check if socket-based activation logic is enabled and at least one fd is provided
    if (sd_listen_fds(0) > 0)
        return QStringLiteral("systemd:");

    return QStringLiteral("unix:path=/run/nemo-devicelock/socket");
}

}

#include "hostservice.moc"
