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

#include "clihostfingerprintsettings.h"

CliHostFingerprintSettings::CliHostFingerprintSettings(QObject *parent)
    :  HostFingerprintSettings(Authenticator::LockCode, parent)
{
}

CliHostFingerprintSettings::~CliHostFingerprintSettings()
{
}

QVector<Fingerprint> CliHostFingerprintSettings::fingerprints() const
{
    return QVector<Fingerprint>();
}

void CliHostFingerprintSettings::remove(const QString &, const QVariant &, const QVariant &)
{
    QDBusContext::connection().send(QDBusContext::message().createErrorReply(QDBusError::NotSupported, QString()));
}

void CliHostFingerprintSettings::rename(const QVariant &, const QString &)
{
    QDBusContext::connection().send(QDBusContext::message().createErrorReply(QDBusError::NotSupported, QString()));
}

CliHostFingerprintSensor::CliHostFingerprintSensor(QObject *parent)
    : HostFingerprintSensor(Authenticator::LockCode, parent)
{
}

CliHostFingerprintSensor::~CliHostFingerprintSensor()
{
}

bool CliHostFingerprintSensor::hasSensor() const
{
    return false;
}

int CliHostFingerprintSensor::acquireFinger(const QString &, const QVariant &)
{

    QDBusContext::connection().send(QDBusContext::message().createErrorReply(QDBusError::NotSupported, QString()));
    return 0;
}

void CliHostFingerprintSensor::cancelAcquisition(const QString &)
{
    QDBusContext::connection().send(QDBusContext::message().createErrorReply(QDBusError::NotSupported, QString()));
}
