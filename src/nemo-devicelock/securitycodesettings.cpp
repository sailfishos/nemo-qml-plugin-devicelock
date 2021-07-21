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
#include <QFileInfo>

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

void SecurityCodeSettingsAdaptor::EncryptionChanged(const QDBusVariant &authenticationToken)
{
    m_settings->handleEncryptionChanged(authenticationToken.variant());
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

/*!
    \class NemoDeviceLock::SecurityCodeSettings
    \brief The SecurityCodeSettings class provides an interface for changing or clearing a security code.
*/

/*!
    Constructs a new security code settings interface which is a child of \a parent.
*/

SecurityCodeSettings::SecurityCodeSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/authenticator"),
          QStringLiteral("org.nemomobile.devicelock.SecurityCodeSettings"))
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_set(false)
    , m_encryptionSet(false)
    , m_changing(false)
    , m_clearing(false)
{
    connect(m_settings.data(), &SettingsWatcher::codeIsMandatoryChanged,
            this, &SecurityCodeSettings::mandatoryChanged);

    connect(m_settings.data(), &SettingsWatcher::alphanumericEncryptionSetChanged,
            this, &SecurityCodeSettings::alphanumericEncryptionSetChanged);

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

/*!
    Destroys a security code settings interface.
*/

SecurityCodeSettings::~SecurityCodeSettings()
{
}

/*!
    \property NemoDeviceLock::SecurityCodeSettings::set

    This property holds whether a security code is currently set.
*/

bool SecurityCodeSettings::isSet() const
{
    return m_set;
}

bool SecurityCodeSettings::isEncryptionSet() const
{
    return m_encryptionSet;
}

bool SecurityCodeSettings::isMandatory() const
{
    return m_settings->codeIsMandatory;
}


bool SecurityCodeSettings::isAlphanumericEncryptionSet() const
{
    return m_settings->alphanumericEncryptionSet;
}

/*!
    Requests a change of the user's security code.

    The security daemon will bring up a dialog prompting the user to change their code.

    This will also authorize a \a challengeCode similar to an Authentication allowing the user
    to enter a new security code and then edit their security settings or also add a fingerprint
    without being prompted for the new code immediately.
*/
void SecurityCodeSettings::change(const QVariant &challengeCode)
{
    handleChange(challengeCode, false);
}

/*!
    Requests a change of the user's LUKS encryption code.

    The security daemon will bring up a dialog prompting the user to change their code.

    This will also authorize a \a challengeCode similar to an Authentication allowing the user
    to enter a new security code and then edit their security settings or also add a fingerprint
    without being prompted for the new code immediately.
*/
void SecurityCodeSettings::changeEncryption(const QVariant &challengeCode)
{
    handleChange(challengeCode, true);
}

void SecurityCodeSettings::handleChange(const QVariant &challengeCode, bool encryption)
{
    if (m_changing) {
        return;
    } else if (m_clearing) {
        cancel();
    }

    m_changing = true;

    auto response = encryption ?
        call(QStringLiteral("ChangeEncryption"), m_localPath, challengeCode) :
        call(QStringLiteral("Change"), m_localPath, challengeCode);

    response->onError([this](const QDBusError &) {
        handleChangeAborted();
    });

    emit changingChanged();
}

/*!
    \signal NemoDeviceLock::SecurityCodeSettings::changed(const QVariant &authenticationToken)

    Signals that the user's security code has been changed and the provided challenge code
    has been authenticated as proven by the \a authenticationToken.
*/

void SecurityCodeSettings::handleChanged(const QVariant &authenticationToken)
{
    if (m_changing) {
        m_changing = false;

        emit changed(authenticationToken);
        emit changingChanged();
    }
}

/*!
    \signal NemoDeviceLock::SecurityCodeSettings::encryptionChanged(const QVariant &authenticationToken)

    Signals that the user's encryption code has been changed and the provided challenge code
    has been authenticated as proven by the \a authenticationToken.
*/

void SecurityCodeSettings::handleEncryptionChanged(const QVariant &authenticationToken)
{
    if (m_changing) {
        m_changing = false;
        emit encryptionChanged(authenticationToken);
        emit changingChanged();
    }
}

/*!
    \signal NemoDeviceLock::SecurityCodeSettings::changeAborted()

    Signals that the user canceled the change of their security code.
*/

void SecurityCodeSettings::handleChangeAborted()
{
    if (m_changing) {
        m_changing = false;

        emit changeAborted();
        emit changingChanged();
    }
}

/*!
    Requests that the users security code be cleared.

    The security daemon will display a prompt asking the user to authenticate themselves before
    proceeding with clearing the security code.
*/

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

/*!
    Cancels an ongoing attempt to change or clear the user's security code.
*/

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

/*!
    \signal NemoDeviceLock::SecurityCodeSettings::cleared()

    Signals that the security code was succesfully cleared.
*/

void SecurityCodeSettings::handleCleared()
{
    if (m_clearing) {
        m_clearing = false;

        emit cleared();
        emit clearingChanged();
    }
}

/*!
    \signal NemoDeviceLock::SecurityCodeSettings::clearAborted()

    Signals that the user canceled clearing their security code.
*/

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

    subscribeToProperty<bool>(QStringLiteral("EncryptionCodeSet"), [this](bool encryptionSet) {
        if (m_encryptionSet != encryptionSet) {
            m_encryptionSet = encryptionSet;
            emit encryptionSetChanged();
        }
    });
}

}
