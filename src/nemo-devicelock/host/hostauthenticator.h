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

#ifndef NEMODEVICELOCK_HOSTAUTHENTICATOR_H
#define NEMODEVICELOCK_HOSTAUTHENTICATOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>

#include <nemo-dbus/interface.h>
#include <nemo-devicelock/host/hostauthenticationinput.h>
#include <nemo-devicelock/host/hostobject.h>

QT_BEGIN_NAMESPACE
class QDBusConnection;
QT_END_NAMESPACE

namespace NemoDeviceLock
{

class HostAuthenticator;
class HostAuthenticatorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(uint AvailableMethods READ availableMethods)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.Authenticator")
public:
    explicit HostAuthenticatorAdaptor(HostAuthenticator *authenticator);

    uint availableMethods() const;

public slots:
    void Authenticate(const QDBusObjectPath &client, const QDBusVariant &challengeCode, uint methods);
    void RequestPermission(
            const QDBusObjectPath &path, const QString &message, const QVariantMap &properties, uint methods);
    void Cancel(const QDBusObjectPath &client);

private:
    HostAuthenticator *m_authenticator;
};

class HostSecurityCodeSettings;
class HostSecurityCodeSettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_PROPERTY(bool SecurityCodeSet READ isSet NOTIFY setChanged)
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.SecurityCodeSettings")
public:
    explicit HostSecurityCodeSettingsAdaptor(HostAuthenticator *authenticator);

    bool isSet() const;

public slots:
    void Change(const QDBusObjectPath &client, const QDBusVariant &challengeCode);
    void CancelChange(const QDBusObjectPath &client);

    void Clear(const QDBusObjectPath &client);
    void CancelClear(const QDBusObjectPath &client);

signals:
    void setChanged();

private:
    HostAuthenticator * const m_authenticator;
};

class HostAuthenticator : public HostAuthenticationInput
{
    Q_OBJECT
public:
    explicit HostAuthenticator(
            Authenticator::Methods supportedMethods = Authenticator::SecurityCode, QObject *parent = nullptr);
    ~HostAuthenticator();

    // Authenticator
    virtual Authenticator::Methods availableMethods() const = 0;
    virtual QVariant authenticateChallengeCode(
            const QVariant &challengeCode, Authenticator::Method method, uint authenticatingPid) = 0;

    // SecurityCodeSettings
    virtual bool authorizeSecurityCodeSettings(unsigned long pid);

    virtual bool clearCode(const QString &code) = 0;

    // AuthenticationInput
    Availability availability(QVariantMap *feedbackData = nullptr) const override = 0;
    int checkCode(const QString &code) override = 0;
    int setCode(const QString &oldCode, const QString &newCode) override = 0;

    void enterSecurityCode(const QString &code) override;
    void requestSecurityCode() override;
    void authorize() override;
    void cancel() override;

    void confirmAuthentication(Authenticator::Method method) override;
    void abortAuthentication(AuthenticationInput::Error error) override;
    void authenticationStarted(
            Authenticator::Methods methods, uint authenticatingPid, AuthenticationInput::Feedback feedback) override;
    void authenticationEnded(bool confirmed) override;

    void setCodeFinished(int result);

    // Signals
    void authenticated(const QVariant &authenticationToken);
    void aborted();

    void securityCodeChanged(const QVariant &authenticationToken);
    void securityCodeChangeAborted();

    void securityCodeCleared();
    void securityCodeClearAborted();

    void availableMethodsChanged();
    void availabilityChanged();

private:
    enum State {
        Idle,
        Authenticating,
        AuthenticationError,
        AuthenticatingForChange,
        RequestingPermission,
        EnteringNewSecurityCode,
        RepeatingNewSecurityCode,
        ExpectingGeneratedSecurityCode,
        Changing,
        ChangeError,
        ChangeCanceled,
        AuthenticatingForClear,
        ClearError,
    };

    inline bool isSecurityCodeSet() const;
    inline void authenticate(
            const QString &authenticator, const QVariant &challengeCode, Authenticator::Methods methods);
    inline void requestPermission(
            const QString &client,
            const QString &message,
            const QVariantMap &properties,
            Authenticator::Methods methods);
    inline void handleChangeSecurityCode(const QString &client, const QVariant &challengeCode);
    inline void handleClearSecurityCode(const QString &client);
    inline void handleCancel(const QString &client);
    inline QVariantMap generatedCodeData();
    inline void enterCodeChangeState(
            FeedbackFunction feedback, Authenticator::Methods methods = Authenticator::Methods());

    friend class HostAuthenticatorAdaptor;
    friend class HostSecurityCodeSettingsAdaptor;

    HostAuthenticatorAdaptor m_adaptor;
    HostSecurityCodeSettingsAdaptor m_securityCodeAdaptor;
    QVariant m_challengeCode;
    QString m_currentCode;
    QString m_newCode;
    QString m_generatedCode;
    int m_repeatsRequired;
    int m_authenticatingPid;
    State m_state;
};

}

#endif
