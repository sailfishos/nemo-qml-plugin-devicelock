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

#ifndef NEMODEVICELOCK_CLIHOSTAUTHENTICATOR_H
#define NEMODEVICELOCK_CLIHOSTAUTHENTICATOR_H

#include <nemo-devicelock/host/hostauthenticator.h>

#include <QDBusConnection>
#include <QSharedDataPointer>

namespace NemoDeviceLock
{

class LockCodeWatcher;

class CliAuthenticator : public HostAuthenticator
{
    Q_OBJECT
public:
    CliAuthenticator(QObject *parent = nullptr);
    ~CliAuthenticator();

    Authenticator::Methods availableMethods() const override;
    Availability availability() const override;

    int checkCode(const QString &code) override;
    int setCode(const QString &oldCode, const QString &newCode) override;
    bool clearCode(const QString &code) override;

    void enterSecurityCode(const QString &code);
    QVariant authenticateChallengeCode(
            const QVariant &challengeCode, Authenticator::Method method, uint authenticatingPid) override;

private:
    QExplicitlySharedDataPointer<LockCodeWatcher> m_watcher;
    QString m_securityCode;
};

}

#endif
