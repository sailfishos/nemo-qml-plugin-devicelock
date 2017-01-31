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

#include <hostfingerprintsensor.h>

namespace NemoDeviceLock
{

static const auto clientInterface = QStringLiteral("org.nemomobile.devicelock.client.Fingerprint.Sensor");

HostFingerprintSensorAdaptor::HostFingerprintSensorAdaptor(HostFingerprintSensor *sensor)
    : QDBusAbstractAdaptor(sensor)
    , m_sensor(sensor)
{
}

bool HostFingerprintSensorAdaptor::hasSensor() const
{
    return m_sensor->hasSensor();
}

uint HostFingerprintSensorAdaptor::AcquireFinger(const QDBusObjectPath &path, const QDBusVariant &authenticationToken)
{
    return m_sensor->acquireFinger(path.path(), authenticationToken.variant());
}

void HostFingerprintSensorAdaptor::CancelAcquisition(const QDBusObjectPath &path)
{
    m_sensor->handleCancel(path.path());
}

HostFingerprintSensor::HostFingerprintSensor(QObject *parent)
    : HostFingerprintSensor(Authenticator::Methods(), parent)
{
}

HostFingerprintSensor::HostFingerprintSensor(Authenticator::Methods allowedMethods, QObject *parent)
    : HostAuthorization(QStringLiteral("/fingerprint/sensor"), allowedMethods, parent)
    , m_adaptor(this)
{
}

HostFingerprintSensor::~HostFingerprintSensor()
{
}

bool HostFingerprintSensor::hasSensor() const
{
    return false;
}

int HostFingerprintSensor::acquireFinger(const QString &, const QVariant &)
{
    QDBusContext::sendErrorReply(QDBusError::NotSupported);
    return 0;
}

void HostFingerprintSensor::cancel()
{
}

void HostFingerprintSensor::sampleAcquired(int samplesRemaining)
{
    sendToActiveClient(clientInterface, QStringLiteral("SampleAcquired"), uint(samplesRemaining));
}

void HostFingerprintSensor::acquisitionCompleted()
{
    sendToActiveClient(clientInterface, QStringLiteral("AcquisitionCompleted"));
}

void HostFingerprintSensor::acquisitionFeedback(FingerprintSensor::Feedback feedback)
{
    sendToActiveClient(clientInterface, QStringLiteral("AcquisitionFeedback"), uint(feedback));
}

void HostFingerprintSensor::acquisitionError(FingerprintSensor::Error error)
{
    sendToActiveClient(clientInterface, QStringLiteral("AcquisitionError"), uint(error));
}

void HostFingerprintSensor::hasSensorChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.Fingerprint.Sensor"),
                QStringLiteral("HasSensor"),
                hasSensor());
}

void HostFingerprintSensor::handleCancel(const QString &client)
{
    if (isActiveClient(client)) {
        cancel();
    }
}

}
