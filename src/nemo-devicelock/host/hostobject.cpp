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

#include "hostobject.h"

#include <QThreadStorage>

#include <dbus/dbus.h>

namespace NemoDeviceLock
{

Q_LOGGING_CATEGORY(daemon, "org.nemomobile.devicelock.daemon", QtCriticalMsg)


class SystemBus : public NemoDBus::Connection
{
public:
    SystemBus()
        : NemoDBus::Connection(QDBusConnection::systemBus(), daemon())
    {
    }
};

NemoDBus::Connection systemBus()
{
    static QThreadStorage<SystemBus> bus;

    return bus.localData();
}

HostObject::HostObject(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
}

HostObject::~HostObject()
{
}

QString HostObject::path() const
{
    return m_path;
}

void HostObject::cancel()
{
}

void HostObject::clientConnected(const QString &connectionName)
{
    m_connections.append(connectionName);
}

void HostObject::clientDisconnected(const QString &connectionName)
{
    m_connections.removeOne(connectionName);

    if (m_activeConnection == connectionName) {
        m_activeConnection.clear();
        m_activeClient.clear();
        cancel();
    }
}

void HostObject::propertyChanged(const QString &interface, const QString &property, const QVariant &value)
{
    qCDebug(daemon, "DBus property changed (%s %s.%s): %s",
            qPrintable(m_path), qPrintable(interface), qPrintable(property), qPrintable(value.toString()));

    const QVariantMap properties = { { property, value } };

    broadcastSignal(
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"),
                NemoDBus::marshallArguments(interface, properties, QStringList()));
}

void HostObject::broadcastSignal(const QString &interface, const QString &name, const QVariantList &arguments)
{
    QDBusMessage message = QDBusMessage::createSignal(m_path, interface, name);

    message.setArguments(arguments);

    for (const auto connectionName : m_connections) {
        QDBusConnection(connectionName).send(message);
    }
}

unsigned long HostObject::connectionPid(const QDBusConnection &connection)
{
    unsigned long pid = 0;
    if (dbus_connection_get_unix_process_id(
                static_cast<DBusConnection *>(connection.internalPointer()), &pid)) {
        return pid;
    } else {
        return 0;
    }
}

unsigned long HostObject::connectionUid(const QDBusConnection &connection)
{
    unsigned long uid = -1;
    if (dbus_connection_get_unix_user(
                static_cast<DBusConnection *>(connection.internalPointer()), &uid)) {
        return uid;
    } else {
        return -1;
    }
}

bool HostObject::authorizeConnection(const QDBusConnection &connection)
{
    Q_UNUSED(connection);

    return true;
}

bool HostObject::isActiveClient(const QString &connection, const QString &client) const
{
    return m_activeConnection == connection && m_activeClient == client;
}

bool HostObject::isActiveClient(const QString &client) const
{
    return isActiveClient(QDBusContext::connection().name(), client);
}

void HostObject::setActiveClient(const QString &connection, const QString &client)
{
    m_activeConnection = connection;
    m_activeClient = client;
}

void HostObject::setActiveClient(const QString &client)
{
    setActiveClient(QDBusContext::connection().name(), client);
}

void HostObject::clearActiveClient()
{
    m_activeConnection.clear();
    m_activeClient.clear();
}

}
