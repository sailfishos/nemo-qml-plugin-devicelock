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

#include "devicereset.h"
#include "private/settingswatcher.h"

namespace NemoDeviceLock
{

/*!
    \class NemoDeviceLock::DeviceReset
    \brief The DeviceReset class provides an interface for requesting the device be reset to factory settings.
*/

/*!
    \enum NemoDeviceLock::DeviceReset::Option

    \value Shutdown Shutdown the device after reset.
    \value Reboot   Reboot the device after reset.
    \value WipePartitions Zero all bytes not overwritten by the factory image.
*/

/*!
    Constructs a new device reset instance which is a child of \a parent.
*/

DeviceReset::DeviceReset(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/devicereset"),
          QStringLiteral("org.nemomobile.devicelock.DeviceReset"))
    , m_authorization(m_localPath, path())
    , m_authorizationAdaptor(&m_authorization, this)
    , m_settings(SettingsWatcher::instance())
{
    connect(m_settings.data(), &SettingsWatcher::supportedDeviceResetOptionsChanged,
            this, &DeviceReset::supportedOptionsChanged);

    m_connection->onConnected(this, [this] {
        connected();
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys a device reset instance.
*/

DeviceReset::~DeviceReset()
{
}

/*!
    \property NemoDeviceLock::DeviceReset::authorization

    This property provides a means of acquiring authorization to reset the device.

*/

Authorization *DeviceReset::authorization()
{
    return &m_authorization;
}

/*!
    \property NemoDeviceLock::DeviceReset::supportedOptions

    This property holds the supported value for the options argument of clear device.

    Values not in this set will be ignored and should not be selectable in the UI.
*/
DeviceReset::Options DeviceReset::supportedOptions() const
{
    return m_settings->supportedDeviceResetOptions;
}

/*!
    Requests a reset of the device to factory settings.

    Use the \a options parameter to supply Additional instructions like whether to reboot the device
    after a reset is compleleted or if the data partitions should be completely wiped .

    The reset authorization challenge code must be authenticated before this is called and the
    \a authenticationToken produced passed as an argument.
*/
void DeviceReset::clearDevice(const QVariant &authenticationToken, Options options)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        auto response = call(QStringLiteral("ClearDevice"), m_localPath, authenticationToken, uint(options));

        response->onFinished([this]() {
            emit clearingDevice();
        });
        response->onError([this](const QDBusError &) {
            emit clearDeviceError();
        });
    }
}

/*!
    \signal NemoDeviceLock::DeviceReset::clearingDevice()

    Signals that the request to clear the device was successful and the action will be performed
    after the device is restarted.
*/

/*!
    \signal NemoDeviceLock::DeviceReset::clearDeviceError()

    Signals that there was an error preventing the device from being reset and no further action
    will be taken.
*/

void DeviceReset::connected()
{
    registerObject();
}

}
