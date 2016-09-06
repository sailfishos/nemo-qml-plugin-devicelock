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

#include "clientdevicelock.h"

#include <QDebug>

ClientDeviceLock::ClientDeviceLock(QObject *parent)
    : DeviceLock(parent)
    , ConnectionClient(
          this, QStringLiteral("/devicelock/lock"), QStringLiteral("org.nemomobile.devicelock.DeviceLock"))
    , m_authorization(m_localPath, m_remotePath)
    , m_authorizationAdaptor(&m_authorization, this)
    , m_state(Undefined)
    , m_enabled(true)
{
    connect(m_connection.data(), &Connection::connected, this, &ClientDeviceLock::connected);

    if (m_connection->isConnected()) {
        connected();
    }
}

ClientDeviceLock::~ClientDeviceLock()
{
}

bool ClientDeviceLock::isEnabled() const
{
    return m_enabled;
}

DeviceLock::LockState ClientDeviceLock::state() const
{
    return m_state;
}

Authorization *ClientDeviceLock::authorization()
{
    return &m_authorization;
}

void ClientDeviceLock::unlock(const QVariant &authenticationToken)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        call(QStringLiteral("Unlock"), m_localPath, authenticationToken);
    }
}

void ClientDeviceLock::connected()
{
    registerObject();

    subscribeToProperty<bool>(QStringLiteral("Enabled"), [this](bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            emit enabledChanged();
        }
    });

    subscribeToProperty<uint>(QStringLiteral("State"), [this](uint state) {
        if (m_state != state) {
            const bool lock = m_state == Unlocked && state == Locked;
            const bool unlock = m_state == Locked && state == Unlocked;

            m_state = LockState(state);

            emit stateChanged();

            if (lock) {
                emit locked();
            } else if (unlock) {
                emit unlocked();
            }
        }
    });
}
