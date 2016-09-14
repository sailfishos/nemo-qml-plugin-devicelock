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

#ifndef NEMODEVICELOCK_FINGERPRINTSETTINGS_H
#define NEMODEVICELOCK_FINGERPRINTSETTINGS_H

#include <nemo-devicelock/private/clientauthorization.h>

#include <QAbstractListModel>
#include <QDateTime>
#include <QDBusArgument>

namespace NemoDeviceLock
{

struct Fingerprint
{
    Fingerprint() = default;
    Fingerprint(const QVariant &id, const QString &name, const QDateTime &acquisitionDate)
        : id(id), name(name), acquisitionDate(acquisitionDate) {}

    QVariant id;
    QString name;
    QDateTime acquisitionDate;
};

class NEMODEVICELOCK_EXPORT FingerprintModel : public QAbstractListModel, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(NemoDeviceLock::Authorization *authorization READ authorization CONSTANT)
public:
    enum {
        PrintId,
        PrintName,
        AcquisitionDate
    };

    explicit FingerprintModel(QObject *parent = nullptr);
    ~FingerprintModel();

    Authorization *authorization();

    Q_INVOKABLE void remove(const QVariant &authenticationToken, const QVariant &id);
    Q_INVOKABLE void rename(const QVariant &id, const QString &name);

    QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

signals:
    void countChanged();

private:
    inline void connected();

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    QVector<Fingerprint> m_fingerprints;
};

class FingerprintSettings;
class FingerprintSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.client.Fingerprint.Sensor")
public:
    explicit FingerprintSettingsAdaptor(FingerprintSettings *settings);

public slots:
    Q_NOREPLY void SampleAcquired(uint samplesRemaining);
    Q_NOREPLY void AcquisitionCompleted();
    Q_NOREPLY void AcquisitionFeedback(uint feedback);
    Q_NOREPLY void AcquisitionError(uint error);

private:
    FingerprintSettings *m_settings;
};

class NEMODEVICELOCK_EXPORT FingerprintSettings : public QObject, private ConnectionClient
{
    Q_OBJECT
    Q_PROPERTY(int samplesRemaining READ samplesRemaining NOTIFY samplesRemainingChanged)
    Q_PROPERTY(int samplesRequired READ samplesRequired NOTIFY samplesRequiredChanged)
    Q_PROPERTY(bool hasSensor READ hasSensor NOTIFY hasSensorChanged)
    Q_PROPERTY(bool acquiring READ isAcquiring NOTIFY acquiringChanged)
    Q_PROPERTY(NemoDeviceLock::Authorization *authorization READ authorization CONSTANT)
    Q_PROPERTY(NemoDeviceLock::FingerprintModel *fingers READ fingers CONSTANT)
    Q_ENUMS(Feedback)
    Q_ENUMS(Error)
public:
    enum Feedback {
        PartialPrint,
        PrintIsUnclear,
        SensorIsDirty,
        SwipeFaster,
        SwipeSlower
    };

    enum Error {
        HardwareUnavailable,
        CannotContinue,
        Timeout,
        NoSpace,
        Canceled
    };

    explicit FingerprintSettings(QObject *parent = nullptr);
    ~FingerprintSettings();

    bool hasSensor() const;
    bool isAcquiring() const;

    Authorization *authorization();

    int samplesRemaining() const;
    int samplesRequired() const;

    Q_INVOKABLE void acquireFinger(const QVariant &authenticationToken);
    Q_INVOKABLE void cancelAcquisition();

    virtual FingerprintModel *fingers();

signals:
    void acquisitionCompleted();
    void acquisitionFeedback(Feedback feedback);
    void acquisitionError(Error error);

    void samplesRemainingChanged();
    void samplesRequiredChanged();
    void hasSensorChanged();
    void acquiringChanged();

private:
    friend class FingerprintSettingsAdaptor;

    inline void connected();
    inline void disconnected();

    inline void handleSampleAcquired(int samplesRemaining);
    inline void handleAcquisitionCompleted();
    inline void handleError(Error error);

    ClientAuthorization m_authorization;
    ClientAuthorizationAdaptor m_authorizationAdaptor;
    FingerprintSettingsAdaptor m_settingsAdaptor;
    FingerprintModel m_fingerprintModel;
    int m_samplesRemaining;
    int m_samplesRequired;
    bool m_hasSensor;
    bool m_isAcquiring;
};

}

Q_DECLARE_METATYPE(NemoDeviceLock::Fingerprint)
Q_DECLARE_METATYPE(QVector<NemoDeviceLock::Fingerprint>)

NEMODEVICELOCK_EXPORT QDBusArgument &operator<<(QDBusArgument &argument, const NemoDeviceLock::Fingerprint &fingerprint);
NEMODEVICELOCK_EXPORT const QDBusArgument &operator>>(const QDBusArgument &argument, NemoDeviceLock::Fingerprint &fingerprint);

#endif
