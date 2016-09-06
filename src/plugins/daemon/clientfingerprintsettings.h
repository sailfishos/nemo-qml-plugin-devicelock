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

#ifndef CLIENTFINGERPRINTSETTINGS_H
#define CLIENTFINGERPRINTSETTINGS_H

#include <fingerprintsettings.h>

#include "clientauthorization.h"

class ClientFingerprintModel : public FingerprintModel, private ConnectionClient
{
    Q_OBJECT
public:
    explicit ClientFingerprintModel(QObject *parent = nullptr);
    ~ClientFingerprintModel();

    Authorization *authorization() override;

    void remove(const QVariant &authenticationToken, const QVariant &id) override;
    void rename(const QVariant &id, const QString &name) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    void connected();

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    QVector<Fingerprint> m_fingerprints;
};

class ClientFingerprintSettings;
class ClientFingerprintSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.client.Fingerprint.Sensor")
public:
    explicit ClientFingerprintSettingsAdaptor(ClientFingerprintSettings *settings);

public slots:
    Q_NOREPLY void SampleAcquired(uint samplesRemaining);
    Q_NOREPLY void AcquisitionCompleted();
    Q_NOREPLY void AcquisitionFeedback(uint feedback);
    Q_NOREPLY void Error(uint error);
    Q_NOREPLY void Canceled();

private:
    ClientFingerprintSettings *m_settings;

};

class ClientFingerprintSettings : public FingerprintSettings, private ConnectionClient
{
    Q_OBJECT
public:
    explicit ClientFingerprintSettings(QObject *parent = nullptr);
    ~ClientFingerprintSettings();

    bool hasSensor() const override;
    bool isAcquiring() const override;

    Authorization *authorization() override;

    int samplesRemaining() const override;
    int samplesRequired() const override;

    void acquireFinger(const QVariant &authenticationToken) override;
    void cancelAcquisition() override;

    FingerprintModel *fingers() override;

private:
    friend class ClientFingerprintSettingsAdaptor;

    void connected();

    void handleSampleAcquired(int samplesRemaining);
    void handleAcquisitionCompleted();
    void handleError(Error error);
    void handleCanceled();

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    ClientFingerprintModel m_fingerprintModel;
    int m_samplesRemaining;
    int m_samplesRequired;
    bool m_hasSensor;
    bool m_isAcquiring;
};

#endif
