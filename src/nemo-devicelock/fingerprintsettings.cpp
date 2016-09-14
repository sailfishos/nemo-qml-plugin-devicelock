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

#include "fingerprintsettings.h"

#include <QDebug>

QDBusArgument &operator<<(QDBusArgument &argument, const NemoDeviceLock::Fingerprint &fingerprint)
{
    argument.beginStructure();
    argument << QDBusVariant(fingerprint.id);
    argument << fingerprint.name;
    argument << fingerprint.acquisitionDate.toString(Qt::ISODate);
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, NemoDeviceLock::Fingerprint &fingerprint)
{
    QDBusVariant id;
    QString acquisitionDate;

    argument.beginStructure();
    argument >> id;
    argument >> fingerprint.name;
    argument >> acquisitionDate;
    argument.endStructure();

    fingerprint.id = id.variant();
    fingerprint.acquisitionDate = QDateTime::fromString(acquisitionDate, Qt::ISODate);

    return argument;
}

namespace NemoDeviceLock
{

FingerprintModel::FingerprintModel(QObject *parent)
    : QAbstractListModel(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/fingerprint/settings"),
          QStringLiteral("org.nemomobile.devicelock.Fingerprint.Settings"))
    , m_authorization(m_localPath, m_remotePath)
    , m_authorizationAdaptor(&m_authorization, this)
{
    connect(m_connection.data(), &Connection::connected, this, &FingerprintModel::connected);

    if (m_connection->isConnected()) {
        connected();
    }
}

FingerprintModel::~FingerprintModel()
{
}

Authorization *FingerprintModel::authorization()
{
    return &m_authorization;
}

void FingerprintModel::remove(const QVariant &authenticationToken, const QVariant &id)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        call(QStringLiteral("Remove"), m_localPath, authenticationToken, id);
    }
}

void FingerprintModel::rename(const QVariant &id, const QString &name)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        call(QStringLiteral("Rename"), id, name);
    }
}

QHash<int, QByteArray> FingerprintModel::roleNames() const
{
    static const QHash<int, QByteArray> roleNames = {
        { PrintId, "fingerprintId" },
        { PrintName, "fingerprintName" },
        { AcquisitionDate, "acquisitionDate" }
    };
    return roleNames;
}

int FingerprintModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_fingerprints.count() : 0;
}

QVariant FingerprintModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= 0 && index.row() < m_fingerprints.count() && index.column() == 0) {
        const auto &fingerprint = m_fingerprints.at(index.row());

        switch (role) {
        case PrintId:
            return fingerprint.id;
        case PrintName:
            return fingerprint.name;
        case AcquisitionDate:
            return fingerprint.acquisitionDate;
        default:
            break;
        }
    }
    return QVariant();
}

void FingerprintModel::connected()
{
    registerObject();

    subscribeToProperty<QVector<Fingerprint>>(
                QStringLiteral("Fingerprints"), [this](const QVector<Fingerprint> fingerprints) {
        int index;
        for (index = 0; index < fingerprints.count(); ++index) {
            const auto &fingerprint = fingerprints.at(index);

            const int previousIndex = [this, fingerprint, index]() {
                for (int previousIndex = index; previousIndex < m_fingerprints.count(); ++previousIndex) {
                    if (m_fingerprints.at(previousIndex).id == fingerprint.id) {
                        return previousIndex;
                    }
                }
                return -1;
            }();

            if (previousIndex == -1) {
                beginInsertRows(QModelIndex(), index, index);
                m_fingerprints.insert(index, fingerprint);
                endInsertRows();
            } else {
                const auto &previousPrint = m_fingerprints.at(previousIndex);

                if (previousPrint.name != fingerprint.name
                        || previousPrint.acquisitionDate != fingerprint.acquisitionDate) {
                    emit dataChanged(createIndex(previousIndex, 0), createIndex(previousIndex, 0));
                }

                if (previousIndex > index) {
                    beginMoveRows(QModelIndex(), previousIndex, previousIndex, QModelIndex(), index);
                    m_fingerprints.removeAt(previousIndex);
                    m_fingerprints.insert(index, fingerprint);
                    endMoveRows();
                }
            }
        }

        if (index < m_fingerprints.count()) {
            beginRemoveRows(QModelIndex(), index, m_fingerprints.count() - 1);
            m_fingerprints.resize(index);
            endRemoveRows();
        }
    });
}

FingerprintSettingsAdaptor::FingerprintSettingsAdaptor(FingerprintSettings *settings)
    : QDBusAbstractAdaptor(settings)
    , m_settings(settings)
{
}

void FingerprintSettingsAdaptor::SampleAcquired(uint samplesRemaining)
{
    m_settings->handleSampleAcquired(samplesRemaining);
}

void FingerprintSettingsAdaptor::AcquisitionCompleted()
{
    m_settings->handleAcquisitionCompleted();
}

void FingerprintSettingsAdaptor::AcquisitionFeedback(uint feedback)
{
    m_settings->acquisitionFeedback(FingerprintSettings::Feedback(feedback));
}

void FingerprintSettingsAdaptor::AcquisitionError(uint error)
{
    m_settings->handleError(FingerprintSettings::Error(error));
}

FingerprintSettings::FingerprintSettings(QObject *parent)
    : QObject(parent)
    , ConnectionClient(
          this,
          QStringLiteral("/fingerprint/sensor"),
          QStringLiteral("org.nemomobile.devicelock.Fingerprint.Sensor"))
    , m_authorization(m_localPath, m_remotePath)
    , m_authorizationAdaptor(&m_authorization, this)
    , m_settingsAdaptor(this)
    , m_samplesRemaining(0)
    , m_samplesRequired(0)
    , m_hasSensor(false)
    , m_isAcquiring(false)
{
    connect(m_connection.data(), &Connection::connected, this, &FingerprintSettings::connected);
    connect(m_connection.data(), &Connection::disconnected, this, &FingerprintSettings::disconnected);

    if (m_connection->isConnected()) {
        connected();
    }
}

FingerprintSettings::~FingerprintSettings()
{
}

bool FingerprintSettings::hasSensor() const
{
    return m_hasSensor;
}

bool FingerprintSettings::isAcquiring() const
{
    return m_isAcquiring;
}

Authorization *FingerprintSettings::authorization()
{
    return &m_authorization;
}

int FingerprintSettings::samplesRemaining() const
{
    return m_samplesRemaining;
}

int FingerprintSettings::samplesRequired() const
{
    return m_samplesRequired;
}

void FingerprintSettings::acquireFinger(const QVariant &authenticationToken)
{
    if (m_authorization.status() == Authorization::ChallengeIssued) {
        m_isAcquiring = true;

        auto response = call(QStringLiteral("AcquireFinger"), m_localPath, authenticationToken);

        response->onFinished<uint>([this](uint samplesRequired) {
            m_samplesRequired = samplesRequired;
            m_samplesRemaining = samplesRequired;

            emit samplesRequiredChanged();
            emit samplesRemainingChanged();
        });

        response->onError([this]() {
            m_isAcquiring = false;

            emit acquisitionError(CannotContinue);
            emit acquiringChanged();
        });

        emit acquiringChanged();
    }
}

void FingerprintSettings::cancelAcquisition()
{
    if (m_isAcquiring) {
        m_isAcquiring = false;

        call(QStringLiteral("CancelAcquisition"), m_localPath);

        emit acquiringChanged();
    }
}

FingerprintModel *FingerprintSettings::fingers()
{
    return &m_fingerprintModel;
}

void FingerprintSettings::handleSampleAcquired(int samplesRemaining)
{
    m_samplesRemaining = samplesRemaining;

    emit samplesRemainingChanged();
}

void FingerprintSettings::handleAcquisitionCompleted()
{
    m_samplesRemaining = 0;
    m_samplesRequired = 0;
    m_isAcquiring = false;

    emit acquisitionCompleted();

    emit acquiringChanged();
    emit samplesRequiredChanged();
    emit samplesRemainingChanged();
}

void FingerprintSettings::handleError(Error error)
{
    m_samplesRemaining = 0;
    m_samplesRequired = 0;
    m_isAcquiring = false;

    emit acquisitionError(error);

    emit acquiringChanged();
    emit samplesRequiredChanged();
    emit samplesRemainingChanged();
}

void FingerprintSettings::connected()
{
    registerObject();
    subscribeToProperty<bool>(QStringLiteral("HasSensor"), [this](bool hasSensor) {
        if (m_hasSensor != hasSensor) {
            m_hasSensor = hasSensor;
            emit hasSensorChanged();
        }
    });
}

void FingerprintSettings::disconnected()
{
    m_samplesRemaining = 0;
    m_samplesRequired = 0;
    m_isAcquiring = false;

    emit acquiringChanged();
    emit samplesRequiredChanged();
    emit samplesRemainingChanged();
}

}
