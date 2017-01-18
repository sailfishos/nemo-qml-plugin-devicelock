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

#include "encryptionsettings.h"
#include "settingswatcher.h"

namespace NemoDeviceLock
{

/*!
    \class NemoDeviceLock::EncryptionSettings
    \brief The EncryptionSettings class provides an interface for changing home folder encryption options.
*/

/*!
    Constructs a new instance of the encryption settings interface which is a child of \a parent.
*/

EncryptionSettings::EncryptionSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/encryption"),
          QStringLiteral("org.nemomobile.devicelock.EncryptionSettings"))
    , m_authorization(m_localPath, path())
    , m_authorizationAdaptor(&m_authorization, this)
    , m_settings(SettingsWatcher::instance())
    , m_supported(false)
{
    m_connection->onConnected(this, [this] {
        connected();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys an encryption settings instance.
*/

EncryptionSettings::~EncryptionSettings()
{
}

/*!
    \property NemoDeviceLock::EncryptionSettings::authorization

    This property provides a means of acquiring authorization to encrypt the home folder.
*/

Authorization *EncryptionSettings::authorization()
{
    return &m_authorization;
}

/*!
    \property NemoDeviceLock::EncryptionSettings::homeEncrypted

    This property holds whether encryption of the home folder has been enabled.
*/

bool EncryptionSettings::isHomeEncrypted() const
{
    return m_settings->isHomeEncrypted;
}

/*!
    \property NemoDeviceLock::EncryptionSettings::supported

    This property holds whether encryption of the home folder is supported.
*/

bool EncryptionSettings::isSupported() const
{
    return m_supported;
}

/*!
    Requests that that the home folder be encrypted.

    The encryption authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/
void EncryptionSettings::encryptHome(const QVariant &authenticationToken)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        auto response = call(QStringLiteral("EncryptHome"),  m_localPath, authenticationToken);

        response->onFinished([this]() {
            emit encryptingHome();
        });
        response->onError([this](const QDBusError &) {
            emit encryptHomeError();
        });
    }
}

/*!
    \signal NemoDeviceLock::EncryptionSettings::encryptingHome()

    Signals that the a request to encrypt the home folder has been successful and the action will
    be performed when the device is restarted.
*/

/*!
    \signal NemoDeviceLock::EncryptionSettings::encryptHomeError()

    Signals that an error occurred when requesting the home folder be encrypted and no further
    action will be taken.
*/

void EncryptionSettings::connected()
{
    void registerObject();

    subscribeToProperty<bool>(QStringLiteral("Supported"), [this](bool supported) {
        m_supported = supported;
    });
}

}
