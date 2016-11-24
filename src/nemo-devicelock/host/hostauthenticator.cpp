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

#include "hostauthenticator.h"

#include "settingswatcher.h"

namespace NemoDeviceLock
{

static const auto authenticatorInterface = QStringLiteral("org.nemomobile.devicelock.client.Authenticator");
static const auto securityCodeInterface = QStringLiteral("org.nemomobile.devicelock.client.SecurityCodeSettings");

HostAuthenticatorAdaptor::HostAuthenticatorAdaptor(HostAuthenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

uint HostAuthenticatorAdaptor::availableMethods() const
{
    return m_authenticator->availableMethods();
}

void HostAuthenticatorAdaptor::Authenticate(
        const QDBusObjectPath &path, const QDBusVariant &challengeCode, uint methods)
{
    m_authenticator->authenticate(
                path.path(), challengeCode.variant(), Authenticator::Methods(methods));
}

void HostAuthenticatorAdaptor::Cancel(const QDBusObjectPath &path)
{
     m_authenticator->handleCancel(path.path());
}

HostSecurityCodeSettingsAdaptor::HostSecurityCodeSettingsAdaptor(HostAuthenticator *authenticator)
    : QDBusAbstractAdaptor(authenticator)
    , m_authenticator(authenticator)
{
}

bool HostSecurityCodeSettingsAdaptor::isSet() const
{
    return m_authenticator->isSecurityCodeSet();
}

void HostSecurityCodeSettingsAdaptor::Change(const QDBusObjectPath &path, const QDBusVariant &challengeCode)
{
    m_authenticator->handleChangeSecurityCode(path.path(), challengeCode.variant());
}

void HostSecurityCodeSettingsAdaptor::CancelChange(const QDBusObjectPath &path)
{
    m_authenticator->handleCancel(path.path());
}

void HostSecurityCodeSettingsAdaptor::Clear(const QDBusObjectPath &path)
{
    m_authenticator->handleClearSecurityCode(path.path());
}

void HostSecurityCodeSettingsAdaptor::CancelClear(const QDBusObjectPath &path)
{
    m_authenticator->handleCancel(path.path());
}

HostAuthenticator::HostAuthenticator(Authenticator::Methods supportedMethods, QObject *parent)
    : HostAuthenticationInput(QStringLiteral("/authenticator"), supportedMethods, parent)
    , m_adaptor(this)
    , m_securityCodeAdaptor(this)
{
}

HostAuthenticator::~HostAuthenticator()
{
}

bool HostAuthenticator::authorizeSecurityCodeSettings(unsigned long)
{
    return true;
}

bool HostAuthenticator::isSecurityCodeSet() const
{
    return availability() != HostAuthenticationInput::AuthenticationNotRequired;
}

void HostAuthenticator::authenticate(
        const QString &client, const QVariant &challengeCode, Authenticator::Methods methods)
{
    cancel();
    setActiveClient(client);

    m_state = Authenticating;
    m_challengeCode = challengeCode;

    switch (availability()) {
    case AuthenticationNotRequired:
        qCDebug(daemon, "Authentication requested. Unsecured, authenticating immediately.");
        confirmAuthentication();
        break;
    case CanAuthenticateSecurityCode:
        methods &= Authenticator::SecurityCode;
        // Fall through.
    case CanAuthenticate:
        qCDebug(daemon, "Authentication requested using methods %i.", int(methods));
        authenticationStarted(methods, AuthenticationInput::EnterSecurityCode);
        break;
    case AuthenticationLocked:
        m_challengeCode.clear();
        authenticationUnavailable(AuthenticationInput::LockedOut);
        break;
    }
}

void HostAuthenticator::handleChangeSecurityCode(const QString &client, const QVariant &challengeCode)
{
    const auto pid = connectionPid(QDBusContext::connection());
    if (pid == 0 || !authorizeSecurityCodeSettings(pid)) {
        QDBusContext::sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    cancel();
    setActiveClient(client);

    m_state = AuthenticatingForChange;
    m_challengeCode = challengeCode;

    switch (availability()) {
    case AuthenticationNotRequired:
        m_state = EnteringNewSecurityCode;
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterNewSecurityCode);
        break;
    case CanAuthenticateSecurityCode:
    case CanAuthenticate:
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterSecurityCode);
        break;
    case AuthenticationLocked:
        m_challengeCode.clear();
        authenticationUnavailable(AuthenticationInput::LockedOut);
        break;
    }

}

void HostAuthenticator::handleClearSecurityCode(const QString &client)
{
    const auto pid = connectionPid(QDBusContext::connection());
    if (pid == 0 || !authorizeSecurityCodeSettings(pid)) {
        QDBusContext::sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    cancel();
    setActiveClient(client);

    m_state = AuthenticatingForClear;

    switch (availability()) {
    case AuthenticationNotRequired:
        m_state = Idle;
        QDBusContext::sendErrorReply(QDBusError::InvalidArgs);
        break;
    case CanAuthenticateSecurityCode:
    case CanAuthenticate:
        authenticationStarted(Authenticator::SecurityCode, AuthenticationInput::EnterSecurityCode);
        break;
    case AuthenticationLocked:
        authenticationUnavailable(AuthenticationInput::LockedOut);
        break;
    }
}

void HostAuthenticator::enterSecurityCode(const QString &code)
{
    int attempts = 0;

    switch (m_state) {
    case Idle:
        return;
    case Authenticating:
        qCDebug(daemon, "Lock code entered for authentication.");
        if ((attempts = checkCode(code)) == Success) {
            confirmAuthentication();
            return;
        }
        break;
    case AuthenticatingForChange:
        qCDebug(daemon, "Lock code entered for code change authentication.");
        if ((attempts = checkCode(code)) == Success) {
            m_state = EnteringNewSecurityCode;
            m_currentCode = code;
            feedback(AuthenticationInput::EnterNewSecurityCode, -1);
            return;
        }
        break;
    case EnteringNewSecurityCode:
        qCDebug(daemon, "New lock code entered.");
        m_newCode = code;
        m_state = RepeatingNewSecurityCode;
        feedback(AuthenticationInput::RepeatNewSecurityCode, -1);
        return;
    case RepeatingNewSecurityCode: {
        qCDebug(daemon, "New lock code confirmation entered.");
        if (m_newCode != code) {
            qCDebug(daemon, "Lock codes don't match.");
            m_currentCode.clear();
            m_newCode.clear();

            feedback(AuthenticationInput::SecurityCodesDoNotMatch, -1);

            if (availability() != AuthenticationNotRequired) {
                m_state = EnteringNewSecurityCode;
                feedback(AuthenticationInput::EnterSecurityCode, -1);
            } else {
                m_state = AuthenticatingForChange;
                feedback(AuthenticationInput::EnterNewSecurityCode, -1);
            }

            return;
        }

        // With disk encryption enabled changing the code can take a few seconds, don't leave
        // the user hanging.
        authenticationEvaluating();

        const auto currentCode = m_currentCode;
        m_currentCode.clear();
        m_newCode.clear();

        switch (setCode(currentCode, code)) {
        case Success:
            qCDebug(daemon, "Lock code changed.");
            securityCodeChanged(authenticateChallengeCode(m_challengeCode));
            return;
        case SecurityCodeInHistory:
            qCDebug(daemon, "Security code disallowed.");
            m_state = EnteringNewSecurityCode;
            feedback(AuthenticationInput::SecurityCodeInHistory, -1);
            feedback(AuthenticationInput::EnterNewSecurityCode, -1);
            return;
        default:
            qCDebug(daemon, "Lock code change failed.");

            m_state = ChangeError;
            authenticationUnavailable(AuthenticationInput::SoftwareError);
            return;
        }
    }
    case AuthenticatingForClear: {
        qCDebug(daemon, "Lock code entered for clear authentication.");
        if ((attempts = checkCode(code)) == 0) {
            if (clearCode(code)) {
                securityCodeCleared();
            } else {
                abortAuthentication(AuthenticationInput::SoftwareError);
            }
            return;
        }
        break;
    }
    default:
        return;
    }

    const int maximum = maximumAttempts();

    if (maximum > 0 && attempts > 0) {
        feedback(AuthenticationInput::IncorrectSecurityCode, qMax(0, maximum - attempts));

        if (attempts >= maximum) {
            abortAuthentication(AuthenticationInput::LockedOut);
            return;
        }
    } else {
        feedback(AuthenticationInput::IncorrectSecurityCode, -1);
    }
}

void HostAuthenticator::confirmAuthentication()
{
    authenticated(authenticateChallengeCode(m_challengeCode));
}

void HostAuthenticator::abortAuthentication(AuthenticationInput::Error error)
{
    switch (m_state) {
    case Authenticating:
        m_state = AuthenticationError;
        break;
    case AuthenticatingForChange:
    case EnteringNewSecurityCode:
    case RepeatingNewSecurityCode:
        m_state = ChangeError;
        break;
    case AuthenticatingForClear:
        m_state = ClearError;
        break;
    default:
        break;
    }

    HostAuthenticationInput::abortAuthentication(error);
}

void HostAuthenticator::authenticationEnded(bool confirmed)
{
    clearActiveClient();

    m_challengeCode.clear();
    m_state = Idle;
    m_currentCode.clear();
    m_newCode.clear();

    HostAuthenticationInput::authenticationEnded(confirmed);
}

void HostAuthenticator::cancel()
{
    switch (m_state) {
    case Authenticating:
    case AuthenticationError:
        aborted();
        break;
    case AuthenticatingForChange:
    case EnteringNewSecurityCode:
    case RepeatingNewSecurityCode:
    case ChangeError:
        securityCodeChangeAborted();
        break;
    case AuthenticatingForClear:
    case ClearError:
        securityCodeClearAborted();
        break;
    default:
        break;
    }
}

void HostAuthenticator::authenticated(const QVariant &authenticationToken)
{
    sendToActiveClient(authenticatorInterface, QStringLiteral("Authenticated"), authenticationToken);
    authenticationEnded(true);
}

void HostAuthenticator::aborted()
{
    sendToActiveClient(authenticatorInterface, QStringLiteral("Aborted"));
    authenticationEnded(false);
}

void HostAuthenticator::securityCodeChanged(const QVariant &authenticationToken)
{
    sendToActiveClient(securityCodeInterface, QStringLiteral("Changed"), authenticationToken);
    authenticationEnded(true);
}

void HostAuthenticator::securityCodeChangeAborted()
{
    sendToActiveClient(securityCodeInterface, QStringLiteral("ChangeAborted"));
    authenticationEnded(false);
}

void HostAuthenticator::securityCodeCleared()
{
    sendToActiveClient(securityCodeInterface, QStringLiteral("Cleared"));
    authenticationEnded(true);
}

void HostAuthenticator::securityCodeClearAborted()
{
    sendToActiveClient(securityCodeInterface, QStringLiteral("ClearAborted"));
    authenticationEnded(false);
}

void HostAuthenticator::availableMethodsChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.Authenticator"),
                QStringLiteral("AvailableMethods"),
                QVariant::fromValue(uint(availableMethods())));
}

void HostAuthenticator::availabilityChanged()
{
    propertyChanged(
                QStringLiteral("org.nemomobile.devicelock.SecurityCodeSettings"),
                QStringLiteral("SecurityCodeSet"),
                QVariant::fromValue(isSecurityCodeSet()));
}

void HostAuthenticator::handleCancel(const QString &client)
{
    if (isActiveClient(client)) {
        cancel();
    }
}

}
