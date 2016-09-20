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

#include "lockcodesettings.h"
#include "settingswatcher.h"

namespace NemoDeviceLock
{

LockCodeSettings::LockCodeSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/lockcode"),
          QStringLiteral("org.nemomobile.devicelock.LockCodeSettings"))
    , m_settings(SettingsWatcher::instance())
    , m_set(false)
{
    connect(m_connection.data(), &Connection::connected, this, &LockCodeSettings::connected);

    if (m_connection->isConnected()) {
        connected();
    }
}

LockCodeSettings::~LockCodeSettings()
{
}

bool LockCodeSettings::isSet() const
{
    return m_set;
}

int LockCodeSettings::minimumLength() const
{
    return m_settings->minimumLength;
}

int LockCodeSettings::maximumLength() const
{
    return m_settings->maximumLength;
}

void LockCodeSettings::change(const QString &oldCode, const QString &newCode)
{
    auto response = call(QStringLiteral("Change"), oldCode, newCode);

    response->onFinished([this]() {
        emit changed();
    });

    response->onError([this](const QDBusError &) {
        emit changeError();
    });
}

void LockCodeSettings::clear(const QString &currentCode)
{
    auto response = call(QStringLiteral("Clear"), currentCode);

    response->onFinished([this]() {
        emit cleared();
    });

    response->onError([this](const QDBusError &) {
        emit clearError();
    });
}

void LockCodeSettings::connected()
{
    subscribeToProperty<bool>(QStringLiteral("LockCodeSet"), [this](bool set) {
        if (m_set != set) {
            m_set = set;
            emit setChanged();
        }
    });
}

}
