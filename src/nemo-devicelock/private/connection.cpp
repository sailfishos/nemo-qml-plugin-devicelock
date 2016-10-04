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

#include <connection.h>
#include "private/logging.h"

#include <QCoreApplication>

namespace NemoDeviceLock
{

Connection *Connection::sharedInstance = nullptr;

static const auto systemdService = QStringLiteral("org.freedesktop.systemd1");

static QDBusConnection connectToHost()
{
    static int counter = 0;

    return QDBusConnection::connectToPeer(
                QStringLiteral("unix:path=/run/nemo-devicelock/socket"),
                QStringLiteral("org.nemomobile.devicelock.%1").arg(counter++));
}

Connection::Connection(QObject *parent)
    : QObject(parent)
    , NemoDBus::Connection(connectToHost(), devicelock())
    , m_systemBus(QDBusConnection::systemBus(), devicelock())
{
    Q_ASSERT(!sharedInstance);
    sharedInstance = this;

    // In the event that the daemon is restarted systemd can tell us when the socket is active
    // and can be connected to again.

    const auto response = m_systemBus.call(
                this,
                systemdService,
                QStringLiteral("/org/freedesktop/systemd1"),
                QStringLiteral("org.freedesktop.systemd1.Manager"),
                QStringLiteral("GetUnit"),
                QStringLiteral("nemo-devicelock.socket"));
    response->onFinished<QDBusObjectPath>([this](const QDBusObjectPath &unit) {
        m_systemBus.subscribeToProperty<QString>(
                    this,
                    systemdService,
                    unit.path(),
                    QStringLiteral("org.freedesktop.systemd1.Unit"),
                    QStringLiteral("ActiveState"),
                    [this] (const QString &state) {
            if (!isConnected() && state == QStringLiteral("active")) {
                qCDebug(devicelock, "The device lock socket is available to connect to");

                reconnect(connectToHost());
            }
        });
    });
}

Connection::~Connection()
{
    QDBusConnection::disconnectFromPeer(connection().name());

    sharedInstance = nullptr;
}

Connection *Connection::instance()
{
    return sharedInstance ? sharedInstance : new Connection;
}

ConnectionClient::ConnectionClient(QObject *context, const QString &path, const QString &interface)
    : ConnectionClient(context, path, interface, generateLocalPath())
{
}

ConnectionClient::ConnectionClient(
        QObject *context,
        const QString &path,
        const QString &interface,
        const QDBusObjectPath &localPath)
    : NemoDBus::Interface(context, *Connection::instance(), QString(), path, interface)
    , m_connection(Connection::instance())
    , m_localPath(localPath)
{
}

void ConnectionClient::registerObject()
{
    m_connection->registerObject(m_localPath.path(), context());
}

QDBusObjectPath ConnectionClient::generateLocalPath()
{
    static const auto pid = QCoreApplication::applicationPid();
    static int objectCounter = 0;

    return QDBusObjectPath(QStringLiteral("/%1/%2").arg(
                QString::number(pid), QString::number(++objectCounter)));
}

}
