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

#ifndef NEMOAUTHENTICATOR_H
#define NEMOAUTHENTICATOR_H

#include <authenticator.h>

#include <MGConfItem>

#include <QSharedDataPointer>

class LockCodeWatcher;

class NemoAuthenticator : public Authenticator
{
    Q_OBJECT
public:
    explicit NemoAuthenticator(QObject *parent = nullptr);
    ~NemoAuthenticator();

    Methods availableMethods() const override;
    Methods utilizedMethods() const override;
    bool isAuthenticating() const override;

    void authenticate(const QVariant &challengeCode, Methods methods) override;
    void enterLockCode(const QString &code) override;
    void cancel() override;

    void abort(Error error);

    static QString pluginName();
    static bool runPlugin(const QStringList &arguments);

protected:
    virtual void authenticationStarted(Methods methods);
    virtual void authenticationEnded(bool confirmed);

    void setUtilizedMethods(Methods methods);
    void confirmAuthentication(const QVariant &authenticationToken);

private:
    QExplicitlySharedDataPointer<LockCodeWatcher> m_watcher;
    MGConfItem m_attemptCount;
    Methods m_utilizedMethods;
    bool m_authenticating;
};

#endif
