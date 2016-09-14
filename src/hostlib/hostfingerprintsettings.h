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

#ifndef HOSTFINGERPRINTSETTINGS_H
#define HOSTFINGERPRINTSETTINGS_H

#include <fingerprintsettings.h>
#include <hostauthorization.h>
#include <dbusutilities.h>

namespace NemoDeviceLock
{

class HostFingerprintSettings;
class HostFingerprintSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(QVector<Fingerprint> Fingerprints READ fingerprints)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.Fingerprint.Settings")
public:
    explicit HostFingerprintSettingsAdaptor(HostFingerprintSettings *settings);

    QVector<Fingerprint> fingerprints() const;

public slots:
    void Remove(
            const QDBusObjectPath &path, const QDBusVariant &authenticationToken, const QDBusVariant &id);
    void Rename(const QDBusVariant &id, const QString &name);

private:
    HostFingerprintSettings * const m_settings;
};

class HostFingerprintSettings : public HostAuthorization
{
    Q_OBJECT
public:
    explicit HostFingerprintSettings(Authenticator::Methods, QObject *parent = nullptr);
    ~HostFingerprintSettings();

protected:
    virtual QVector<Fingerprint> fingerprints() const = 0;

    virtual void remove(
            const QString &requestor, const QVariant &authenticationToken, const QVariant &id) = 0;
    virtual void rename(const QVariant &id, const QString &name) = 0;

    void fingerprintsChanged();

private:
    friend class HostFingerprintSettingsAdaptor;

    HostFingerprintSettingsAdaptor m_adaptor;
};

class HostFingerprintSensor;
class HostFingerprintSensorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(bool HasSensor READ hasSensor)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.Fingerprint.Sensor")
public:
    explicit HostFingerprintSensorAdaptor(HostFingerprintSensor *sensor);

    bool hasSensor() const;

public slots:
    uint AcquireFinger(const QDBusObjectPath &path, const QDBusVariant &authenticationToken);
    void CancelAcquisition(const QDBusObjectPath &path);

private:
    HostFingerprintSensor * const m_sensor;
};

class HostFingerprintSensor : public HostAuthorization
{
    Q_OBJECT
public:
    explicit HostFingerprintSensor(Authenticator::Methods allowedMethods, QObject *parent = nullptr);
    ~HostFingerprintSensor();

protected:
    virtual bool hasSensor() const = 0;

    virtual int acquireFinger(const QString &requestor, const QVariant &authenticationToken) = 0;
    virtual void cancelAcquisition(const QString &requestor) = 0;

    void sendSampleAcquired(const QString &connection, const QString &path, int samplesRemaining);
    void sendAcquisitionCompleted(const QString &connection, const QString &path);
    void sendAcquisitionFeedback(const QString &connection, const QString &path, FingerprintSettings::Feedback feedback);
    void sendAcquisitionError(const QString &connection, const QString &path, FingerprintSettings::Error error);
    void sendCanceled(const QString &connection, const QString &path);

    void hasSensorChanged();

private:
    friend class HostFingerprintSensorAdaptor;

    HostFingerprintSensorAdaptor m_adaptor;
};

}

#endif
