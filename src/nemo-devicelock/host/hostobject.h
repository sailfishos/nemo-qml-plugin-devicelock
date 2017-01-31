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

#ifndef NEMODEVICELOCK_HOSTOBJECT_H
#define NEMODEVICELOCK_HOSTOBJECT_H

#include <QDBusContext>
#include <QLoggingCategory>

#include <nemo-dbus/connection.h>

namespace NemoDeviceLock
{

Q_DECLARE_LOGGING_CATEGORY(daemon)

NemoDBus::Connection systemBus();

class HostObject : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit HostObject(const QString &path, QObject *parent = nullptr);
    ~HostObject();

    QString path() const;

    virtual void clientConnected(const QString &connectionName);
    virtual void clientDisconnected(const QString &connectionName);

    virtual void cancel();

    static unsigned long connectionPid(const QDBusConnection &connection);
    static unsigned long connectionUid(const QDBusConnection &connection);

    virtual bool authorizeConnection(const QDBusConnection &connection);

    bool isActiveClient(const QString &client) const;
    void setActiveClient(const QString &client);
    void clearActiveClient();

protected:
    void propertyChanged(const QString &interface, const QString &property, const QVariant &value);

    template <typename... Arguments> inline bool sendToActiveClient(
                const QString &interface,
                const QString &method,
                Arguments... arguments)
    {
        return !m_activeConnection.isEmpty()
                && NemoDBus::send(m_activeConnection, m_activeClient, interface, method, arguments...);
    }

private:
    const QString m_path;
    QStringList m_connections;
    QString m_activeConnection;
    QString m_activeClient;
};

}

#endif
