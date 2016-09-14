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

#include <QCoreApplication>

#include <QDebug>

namespace NemoDeviceLock
{

PendingCall::PendingCall(
        const QDBusPendingCall &call, const QString &path, const QString &interface, const QString &method, QObject *parent)
    : QDBusPendingCallWatcher(call, parent)
    , m_path(path)
    , m_interface(interface)
    , m_method(method)
{
}

PendingCall::~PendingCall()
{
}

PropertyChanges::PropertyChanges(
        Connection *cache, const QDBusConnection &connection, const QString &service, const QString &path)
    : QObject(cache)
    , m_cache(cache)
    , m_connection(connection)
    , m_service(service)
    , m_path(path)
{
}

PropertyChanges::~PropertyChanges()
{
}

void PropertyChanges::addSubscriber(QObject *subscriber)
{
    if (!m_subscribers.contains(subscriber)) {
        connect(subscriber, &QObject::destroyed, this, &PropertyChanges::subscriberDestroyed);

        m_subscribers.append(subscriber);
    }
}

void PropertyChanges::subscriberDestroyed(QObject *subscriber)
{
    m_subscribers.removeOne(subscriber);

    if (m_subscribers.isEmpty()) {
        for (auto &connections : m_cache->m_propertyChanges) {
            for (auto &services : connections) {
                for (auto it = services.begin(); it != services.end(); ++it) {
                    if (*it == this) {
                        services.erase(it);
                        break;
                    }
                }
            }
        }
        delete this;
    }
}

void PropertyChanges::getProperty(const QString &interface, const QString &property)
{
    auto response = m_cache->call(
                this,
                m_connection,
                m_service,
                m_path,
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("Get"),
                marshallArguments(interface, property));

    response->onFinished<QDBusVariant>([this, interface, property](const QDBusVariant &value) {
        emit propertyChanged(interface, property, value.variant());
    });
}

void PropertyChanges::propertiesChanged(
        const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    for (auto it = changed.begin(); it!= changed.end(); ++it) {
        emit propertyChanged(interface, it.key(), it.value());
    }

    for (auto property : invalidated) {
        getProperty(interface, property);
    }
}

Connection *Connection::sharedInstance = nullptr;

static const auto systemdService = QStringLiteral("org.freedesktop.systemd1");

static QDBusConnection connectToHost()
{
    static int counter = 0;

    return QDBusConnection::connectToPeer(
                QStringLiteral("unix:path=/run/org-nemomobile-devicelock.socket"),
                QStringLiteral("org.nemomobile.devicelock.%1").arg(counter++));
}

Connection::Connection(QObject *parent)
    : QObject(parent)
    , m_connection(connectToHost())
{
    Q_ASSERT(!sharedInstance);
    sharedInstance = this;

    if (m_connection.isConnected()) {
        connectToDisconnected();
    }

    // In the event that the daemon is restarted systemd can tell us when the socket is active
    // and can be connected to again.
    const auto response = call(
                this,
                QDBusConnection::systemBus(),
                systemdService,
                QStringLiteral("/org/freedesktop/systemd1"),
                QStringLiteral("org.freedesktop.systemd1.Manager"),
                QStringLiteral("GetUnit"),
                marshallArguments(QStringLiteral("nemodevicelock.socket")));
    response->onFinished<QDBusObjectPath>([this](const QDBusObjectPath &unit) {
        subscribeToProperty<QString>(
                    this,
                    QDBusConnection::systemBus(),
                    systemdService,
                    unit.path(),
                    QStringLiteral("org.freedesktop.systemd1.Unit"),
                    QStringLiteral("ActiveState"),
                    [this] (const QString &state) {
            if (!m_connection.isConnected() && state == QStringLiteral("active")) {
                m_connection = connectToHost();

                if (m_connection.isConnected()) {
                    connectToDisconnected();
                    emit connected();
                }
            }
        });
    });
}

Connection::~Connection()
{
    const auto connections = m_propertyChanges;
    m_propertyChanges.clear();

    for (auto services : connections) {
        for (auto propertyChanges : services) {
            qDeleteAll(propertyChanges);
        }
    }

    QDBusConnection::disconnectFromPeer(m_connection.name());

    sharedInstance = nullptr;
}

Connection *Connection::instance()
{
    return sharedInstance ? sharedInstance : new Connection;
}

bool Connection::isConnected() const
{
    return m_connection.isConnected();
}

bool Connection::getProperty(
        QVariant *value,
        const QDBusConnection &connection,
        const QString &service,
        const QString &path,
        const QString &interface,
        const QString &property)
{
    auto message = QDBusMessage::createMethodCall(
                service, path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    message.setArguments(marshallArguments(interface, property));

    const auto reply = connection.call(message);

    if (reply.type() == QDBusMessage::ReplyMessage) {
        *value = reply.arguments().value(0).value<QDBusVariant>().variant();

        return true;
    } else {
        qWarning() << "Failed to query property" << reply.errorMessage();
        return false;
    }
}

PendingCall *Connection::call(
        QObject *caller,
        const QDBusConnection &connection,
        const QString &service,
        const QString &path,
        const QString &interface,
        const QString &method,
        const QVariantList &arguments)
{
    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(arguments);

    const auto call = new PendingCall(connection.asyncCall(message), path, interface, method, caller);

    connect(call, &QDBusPendingCallWatcher::finished, this, &Connection::callFinished);

    return call;
}

PropertyChanges *Connection::subscribeToObject(
        QObject *caller, const QDBusConnection &connection, const QString &service, const QString &path)
{
    auto &propertyChanges = m_propertyChanges[connection.name()][service][path];
    if (!propertyChanges) {
        propertyChanges = new PropertyChanges(this, connection, service, path);
        QDBusConnection(connection).connect(
                    QString(),
                    path,
                    QStringLiteral("org.freedesktop.DBus.Properties"),
                    QStringLiteral("PropertiesChanged"),
                    propertyChanges,
                    SLOT(propertiesChanged(QString,QVariantMap,QStringList)));
    }

    propertyChanges->addSubscriber(caller);

    return propertyChanges;
}

void Connection::registerObject(const QString &path, QObject *object)
{
    if (!m_connection.registerObject(path, object)) {
        qWarning() << "Failed to register object on path" << path << object;
    }
}

void Connection::callFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();

    const QDBusPendingCall reply = *watcher;
    const auto call = static_cast<PendingCall *>(watcher);

    if (reply.isError()) {
        qWarning() << "there was an error" << call->path() << call->interface() << call->method() << call->error().message();
        call->failure(reply.error());
    } else {
        call->success(reply);
    }
}

void Connection::connectToDisconnected()
{
    if (!m_connection.connect(
                QString(),
                QStringLiteral("/org/freedesktop/DBus/Local"),
                QStringLiteral("org.freedesktop.DBus.Local"),
                QStringLiteral("Disconnected"),
                this,
                SIGNAL(handleDisconnect()))) {
        qWarning() << "Failed to connect to disconnected signal" << m_connection.lastError();
    }
}

void Connection::handleDisconnect()
{
    deletePropertyListeners();

    emit disconnected();
}

void Connection::deletePropertyListeners()
{
    const auto services = m_propertyChanges.take(m_connection.name());

    for (auto propertyChanges : services) {
        qDeleteAll(propertyChanges);
    }
}

ConnectionClient::ConnectionClient(QObject *caller, const QString &path, const QString &interface)
    : m_connection(Connection::instance())
    , m_caller(caller)
    , m_remotePath(path)
    , m_remoteInterface(interface)
    , m_localPath(generateLocalPath())
{
}

ConnectionClient::ConnectionClient(
        QObject *caller,
        const QString &path,
        const QString &interface,
        const QDBusObjectPath &localPath)
    : m_connection(Connection::instance())
    , m_caller(caller)
    , m_remotePath(path)
    , m_remoteInterface(interface)
    , m_localPath(localPath)
{
}

void ConnectionClient::registerObject()
{
    m_connection->registerObject(m_localPath.path(), m_caller);
}

QDBusObjectPath ConnectionClient::generateLocalPath()
{
    static const auto pid = QCoreApplication::applicationPid();
    static int objectCounter = 0;

    return QDBusObjectPath(QStringLiteral("/%1/%2").arg(
                QString::number(pid), QString::number(++objectCounter)));
}

}
