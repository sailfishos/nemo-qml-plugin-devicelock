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

#include "securitycodesettings.h"
#include "settingswatcher.h"

namespace NemoDeviceLock
{

SecurityCodeSettingsAdaptor::SecurityCodeSettingsAdaptor(SecurityCodeSettings *settings)
    : QDBusAbstractAdaptor(settings)
    , m_settings(settings)
{
}

void SecurityCodeSettingsAdaptor::Changed(const QDBusVariant &authenticationToken)
{
    m_settings->handleChanged(authenticationToken.variant());
}

void SecurityCodeSettingsAdaptor::ChangeAborted()
{
    m_settings->handleChangeAborted();
}

void SecurityCodeSettingsAdaptor::Cleared()
{
    m_settings->handleCleared();
}

void SecurityCodeSettingsAdaptor::ClearAborted()
{
    m_settings->handleClearAborted();
}

SecurityCodeSettings::SecurityCodeSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/authenticator"),
          QStringLiteral("org.nemomobile.devicelock.SecurityCodeSettings"))
    , m_adaptor(this)
    , m_set(false)
    , m_changing(false)
    , m_clearing(false)
{
    m_connection->onConnected(this, [this] {
        connected();
    });

    m_connection->onDisconnected(this, [this] {
        handleChangeAborted();
        handleClearAborted();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

SecurityCodeSettings::~SecurityCodeSettings()
{
}

bool SecurityCodeSettings::isSet() const
{
    return m_set;
}

void SecurityCodeSettings::change(const QVariant &authenticationCode)
{
    if (m_changing) {
        return;
    } else if (m_clearing) {
        cancel();
    }

    m_changing = true;

    auto response = call(QStringLiteral("Change"), m_localPath, authenticationCode);

    response->onError([this](const QDBusError &) {
        handleChangeAborted();
    });

    emit changingChanged();
}

void SecurityCodeSettings::handleChanged(const QVariant &authenticationToken)
{
    if (m_changing) {
        m_changing = false;

        emit changed(authenticationToken);
        emit changingChanged();
    }
}

void SecurityCodeSettings::handleChangeAborted()
{
    if (m_changing) {
        m_changing = false;

        emit changeAborted();
        emit changingChanged();
    }
}

void SecurityCodeSettings::clear()
{
    if (m_changing) {
        cancel();
    } else if (m_clearing) {
        return;
    }

    m_clearing = true;

    auto response = call(QStringLiteral("Clear"), m_localPath);

    response->onError([this](const QDBusError &) {
        handleClearAborted();
    });
}

void SecurityCodeSettings::cancel()
{
    if (m_changing) {
        m_changing = false;

        call(QStringLiteral("CancelChange"), m_localPath);

        emit changingChanged();
    } else if (m_clearing) {
        m_clearing = false;

        call(QStringLiteral("CancelClear"), m_localPath);

        emit clearingChanged();
    }
}

void SecurityCodeSettings::handleCleared()
{
    if (m_clearing) {
        m_clearing = false;

        emit cleared();
        emit clearingChanged();
    }
}

void SecurityCodeSettings::handleClearAborted()
{
    if (m_clearing) {
        m_clearing = false;

        emit clearAborted();
        emit clearingChanged();
    }
}

void SecurityCodeSettings::connected()
{
    registerObject();
    subscribeToProperty<bool>(QStringLiteral("SecurityCodeSet"), [this](bool set) {
        if (m_set != set) {
            m_set = set;
            emit setChanged();
        }
    });
}

}
