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

#ifndef FINGERPRINTSETTINGS_H
#define FINGERPRINTSETTINGS_H

#include <QAbstractListModel>

class Authorization;

class FingerprintModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(Authorization *authorization READ authorization CONSTANT)
public:
    enum {
        PrintId,
        PrintName,
        AcquisitionDate
    };

    explicit FingerprintModel(QObject *parent = nullptr);
    ~FingerprintModel();

    virtual Authorization *authorization() = 0;

    Q_INVOKABLE virtual void remove(const QVariant &authenticationToken, const QVariant &id) = 0;
    Q_INVOKABLE virtual void rename(const QVariant &id, const QString &name) = 0;

    QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override = 0;
    QVariant data(const QModelIndex &index, int role) const override = 0;

signals:
    void countChanged();
};

class FingerprintSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int samplesRemaining READ samplesRemaining NOTIFY samplesRemainingChanged)
    Q_PROPERTY(int samplesRequired READ samplesRequired NOTIFY samplesRequiredChanged)
    Q_PROPERTY(bool hasSensor READ hasSensor NOTIFY hasSensorChanged)
    Q_PROPERTY(bool acquiring READ isAcquiring NOTIFY acquiringChanged)
    Q_PROPERTY(Authorization *authorization READ authorization CONSTANT)
    Q_PROPERTY(FingerprintModel *fingers READ fingers CONSTANT)
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

    virtual bool hasSensor() const = 0;
    virtual bool isAcquiring() const = 0;

    virtual Authorization *authorization() = 0;

    virtual int samplesRemaining() const = 0;
    virtual int samplesRequired() const = 0;

    Q_INVOKABLE virtual void acquireFinger(const QVariant &authenticationToken) = 0;
    Q_INVOKABLE virtual void cancelAcquisition() = 0;

    virtual FingerprintModel *fingers() = 0;

signals:
    void acquisitionCompleted();
    void acquisitionFeedback(Feedback feedback);
    void acquisitionError(Error error);

    void samplesRemainingChanged();
    void samplesRequiredChanged();
    void hasSensorChanged();
    void acquiringChanged();
};

#endif
