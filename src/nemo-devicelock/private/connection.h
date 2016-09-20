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

#ifndef NEMODEVICELOCK_CONNECTION_H
#define NEMODEVICELOCK_CONNECTION_H

#include <nemo-devicelock/private/dbusutilities.h>

#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusVariant>
#include <QSharedData>

namespace NemoDeviceLock
{

class PendingCall : public QDBusPendingCallWatcher
{
    Q_OBJECT

    template <typename Handler> static void invoke(const Handler &handler, const QDBusPendingCall &) { handler(); }
    template <typename Handler, typename Argument0> static void invoke(
            const Handler &handler, const QDBusPendingCall &call) {
        const QDBusPendingReply<Argument0> reply(call);
        handler(demarshallArgument<Argument0>(reply.argumentAt(0))); }
    template <typename Handler, typename Argument0, typename Argument1> static void invoke(
            const Handler &handler, const QDBusPendingCall &call) {
        const QDBusPendingReply<Argument0, Argument1> reply(call);
        handler(demarshallArgument<Argument0>(reply.argumentAt(0)), demarshallArgument<Argument1>(reply.argumentAt(1))); }

public:
    template <typename... Arguments, typename Handler>
    void onFinished(const Handler &handler)
    {
        connect(this, &PendingCall::success, [handler](const QDBusPendingCall &call) {
            invoke<Handler, Arguments...>(handler, call);
        });
    }

    template <typename T> void onError(const T &handler)
    {
        connect(this, &PendingCall::failure, [handler](const QDBusError &error) {
            handler(error);
        });
    }

    QString path() const { return m_path; }
    QString interface() const { return m_interface; }
    QString method() const { return m_method; }

signals:
    void success(const QDBusPendingCall &call);
    void failure(const QDBusError &error);

private:
    friend class Connection;

    explicit PendingCall(const QDBusPendingCall &call, const QString &path, const QString &interface, const QString &method, QObject *parent);
    ~PendingCall();

    QString m_path;
    QString m_interface;
    QString m_method;
};

class Connection;

class PropertyChanges : public QObject
{
    Q_OBJECT
public:
    ~PropertyChanges();

signals:
    void propertyChanged(const QString &interface, const QString &property, const QVariant &value);

private slots:
    void propertiesChanged(
            const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    friend class Connection;

    PropertyChanges(
            Connection *cache,
            const QDBusConnection &connection,
            const QString &service,
            const QString &path);

    void addSubscriber(QObject *subscriber);
    void subscriberDestroyed(QObject *subscriber);
    void getProperty(const QString &interface, const QString &property);

    Connection * const m_cache;
    QList<QObject *> m_subscribers;
    QDBusConnection m_connection;
    QString m_service;
    QString m_path;
};

class Connection : public QObject, public QSharedData
{
    Q_OBJECT
public:
    ~Connection();

    static Connection *instance();

    bool isConnected() const;

    bool getProperty(
            QVariant *value,
            const QDBusConnection &connection,
            const QString &service,
            const QString &path,
            const QString &interface,
            const QString &property);

    PendingCall *call(
            QObject *caller,
            const QDBusConnection &connection,
            const QString &service,
            const QString &path,
            const QString &interface,
            const QString &method,
            const QVariantList &arguments);

    PendingCall *call(
            QObject *caller,
            const QString &path,
            const QString &interface,
            const QString &method,
            const QVariantList &arguments) {
        return call(caller, m_connection, QString(), path, interface, method, arguments);
    }
    PropertyChanges *subscribeToObject(
            QObject *caller, const QDBusConnection &connection, const QString &service, const QString &path);

    template <typename T, typename Handler>
    void subscribeToProperty(
            QObject *caller,
            const QDBusConnection &connection,
            const QString &service,
            const QString &path,
            const QString &interface,
            const QString &property,
            const Handler &onChanged)
    {
        const auto propertyChanges = subscribeToObject(
                    caller, connection, service, path);
        QObject::connect(
                    propertyChanges, &PropertyChanges::propertyChanged,
                    [interface, property, onChanged](
                        const QString changedInterface, const QString &changedProperty, const QVariant &value) {
            if (interface == changedInterface && property == changedProperty) {
                onChanged(demarshallArgument<T>(value));
            }
        });

        QVariant value;
        if (getProperty(&value, connection, service, path, interface, property)) {
            onChanged(demarshallArgument<T>(value));
        }
    }

    template <typename T, typename Handler>
    void subscribeToProperty(
            QObject *caller,
            const QString &path,
            const QString &interface,
            const QString &property,
            const Handler &onChanged)
    {
        return subscribeToProperty<T>(caller, m_connection, QString(), path, interface, property, onChanged);
    }

    void registerObject(const QString &path, QObject *object);

signals:
    void connected();
    void disconnected();

private slots:
    void handleDisconnect();

private:
    friend class PropertyChanges;

    explicit Connection(QObject *parent = nullptr);

    void connectToDisconnected();
    void deletePropertyListeners();

    void callFinished(QDBusPendingCallWatcher *watcher);

    QDBusConnection m_connection;
    QHash<QString, QHash<QString, QHash<QString, PropertyChanges *>>> m_propertyChanges;

    static Connection *sharedInstance;
};

class ConnectionClient
{
protected:
    ConnectionClient(QObject *caller, const QString &path, const QString &interface);
    ConnectionClient(
            QObject *caller,
            const QString &path,
            const QString &interface,
            const QDBusObjectPath &localPath);
    virtual ~ConnectionClient() {}

    template <typename... Arguments>
    PendingCall *call(const QString &method, Arguments... arguments)
    {
        return m_connection->call(
                    m_caller, m_remotePath, m_remoteInterface, method, marshallArguments(arguments...));
    }

    template <typename T, typename Handler>
    void subscribeToProperty(const QString &property, const Handler &onChanged)
    {
        return m_connection->subscribeToProperty<T>(
                    m_caller, m_remotePath, m_remoteInterface, property, onChanged);
    }

    void registerObject();

    QExplicitlySharedDataPointer<Connection> m_connection;
    QObject *m_caller;
    QString m_remotePath;
    QString m_remoteInterface;
    QDBusObjectPath m_localPath;

private:
    static QDBusObjectPath generateLocalPath();
};

}

#endif
