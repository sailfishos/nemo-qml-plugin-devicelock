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

#include <hostfingerprintsettings.h>

namespace NemoDeviceLock
{

static const auto clientInterface = QStringLiteral("org.nemomobile.devicelock.client.Fingerprint.Sensor");

HostFingerprintSettingsAdaptor::HostFingerprintSettingsAdaptor(HostFingerprintSettings *settings)
    : QDBusAbstractAdaptor(settings)
    , m_settings(settings)
{
}

QVector<Fingerprint> HostFingerprintSettingsAdaptor::fingerprints() const
{
    return m_settings->fingerprints();
}

void HostFingerprintSettingsAdaptor::Remove(
        const QDBusObjectPath &path, const QDBusVariant &authenticationToken, const QDBusVariant &id)
{
    m_settings->remove(path.path(), authenticationToken.variant(), id.variant());
}

void HostFingerprintSettingsAdaptor::Rename(const QDBusVariant &id, const QString &name)
{
    m_settings->rename(id.variant(), name);
}

HostFingerprintSettings::HostFingerprintSettings(QObject *parent)
    : HostFingerprintSettings(Authenticator::Methods(), parent)
{
}

HostFingerprintSettings::HostFingerprintSettings(Authenticator::Methods allowedMethods, QObject *parent)
    : HostAuthorization(QStringLiteral("/fingerprint/settings"), allowedMethods, parent)
    , m_adaptor(this)
{
}

HostFingerprintSettings::~HostFingerprintSettings()
{
}

QVector<Fingerprint> HostFingerprintSettings::fingerprints() const
{
    return QVector<Fingerprint>();
}

void HostFingerprintSettings::remove(const QString &, const QVariant &, const QVariant &)
{
    QDBusContext::sendErrorReply(QDBusError::NotSupported);
}

void HostFingerprintSettings::rename(const QVariant &, const QString &)
{
    QDBusContext::sendErrorReply(QDBusError::NotSupported);
}

void HostFingerprintSettings::fingerprintsChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.Fingerprint.Settings"),
                QStringLiteral("Fingerprints"),
                QVariant::fromValue(fingerprints()));
}

}
