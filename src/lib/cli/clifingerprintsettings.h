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

#ifndef CLIFINGERPRINTSETTINGS_H
#define CLIFINGERPRINTSETTINGS_H

#include <fingerprintsettings.h>

#include <cliauthorization.h>

#include <QSharedDataPointer>

class CliFingerprintModel : public FingerprintModel
{
    Q_OBJECT
public:
    explicit CliFingerprintModel(QObject *parent = nullptr);
    ~CliFingerprintModel();

    Authorization *authorization() override;

    void remove(const QVariant &authenticationToken, const QVariant &id) override;
    void rename(const QVariant &id, const QString &name) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    CliAuthorization m_authorization;
};

class CliFingerprintSettings : public FingerprintSettings
{
    Q_OBJECT
public:
    explicit CliFingerprintSettings(QObject *parent = nullptr);
    ~CliFingerprintSettings();

    bool hasSensor() const override;
    bool isAcquiring() const override;

    int samplesRemaining() const override;
    int samplesRequired() const override;

    Authorization *authorization() override;

    void acquireFinger(const QVariant &authenticationToken) override;
    void cancelAcquisition() override;

    FingerprintModel *fingers() override;

private:
    CliAuthorization m_authorization;
    CliFingerprintModel m_model;
};

#endif
