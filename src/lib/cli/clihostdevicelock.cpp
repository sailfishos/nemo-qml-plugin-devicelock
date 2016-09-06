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

#include "clihostdevicelock.h"

#include "lockcodewatcher.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <QDebug>

CliHostDeviceLock::CliHostDeviceLock(QObject *parent)
    : HostDeviceLock(Authenticator::LockCode, parent)
    , m_watcher(LockCodeWatcher::instance())
{
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &CliHostDeviceLock::enabledChanged);
    connect(&m_mce, &MceDeviceLock::stateChanged, this, &CliHostDeviceLock::stateChanged);

    m_mce.init();
}

CliHostDeviceLock::~CliHostDeviceLock()
{
}

DeviceLock::LockState CliHostDeviceLock::state() const
{
    return m_mce.state();
}

bool CliHostDeviceLock::isEnabled() const
{
    return m_watcher->lockCodeSet();
}

void CliHostDeviceLock::unlock(const QString &, const QVariant &authenticationToken)
{
    if (PluginCommand *command = m_watcher->unlock(this, authenticationToken.toString())) {
        auto connection = QDBusContext::connection();
        auto message = QDBusContext::message();

        QDBusContext::setDelayedReply(true);

        command->onSuccess([this, connection, message]() {
            connection.send(message.createReply());

            m_mce.setState(DeviceLock::Unlocked);
        });

        command->onFailure([this, connection, message]() {
            connection.send(message.createErrorReply(QDBusError::AccessDenied, QString()));
        });
    } else {
        QDBusContext::sendErrorReply(QDBusError::InternalError);
    }
}
