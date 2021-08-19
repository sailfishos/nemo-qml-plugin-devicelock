/*
 * Copyright (c) 2016 - 2021 Jolla Ltd
 * Copyright (c) 2021 Open Mobile Platform LLC
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

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>

#include <unistd.h>

namespace NemoDeviceLock
{

static const auto authenticatorInterface = QStringLiteral("org.nemomobile.devicelock.client.Authenticator");
static const auto securityCodeInterface = QStringLiteral("org.nemomobile.devicelock.client.SecurityCodeSettings");
static const auto attemptsRemaining = QStringLiteral("attemptsRemaining");

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

void HostAuthenticatorAdaptor::RequestPermission(
        const QDBusObjectPath &path, const QString &message, const QVariantMap &properties, uint methods)
{
    m_authenticator->requestPermission(path.path(), message, properties, Authenticator::Methods(methods));
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
    , m_repeatsRequired(0)
    , m_authenticatingPid(0)
    , m_state(Idle)
{
    systemBus().registerObject(path(), this);
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
    switch (availability()) {
    case HostAuthenticationInput::AuthenticationNotRequired:
    case HostAuthenticationInput::SecurityCodeRequired:
        return false;
    default:
        return true;
    }
}

void HostAuthenticator::authenticate(
        const QString &client, const QVariant &challengeCode, Authenticator::Methods methods)
{
    const auto pid = connectionPid(QDBusContext::connection());

    cancelPending();

    if (m_state == Idle) {
        setActiveClient(client);
        beginAuthenticate(pid, challengeCode, methods);
    } else {
        m_pending.connection = QDBusContext::connection().name();
        m_pending.client = client;
        m_pending.address = QDBusContext::message().service();
        m_pending.pid = pid;
        m_pending.request = AuthenticateRequest;
        m_pending.challengeCode = challengeCode;
        m_pending.methods = methods;
        cancel();
    }
}

void HostAuthenticator::beginAuthenticate(
            uint pid, const QVariant &challengeCode, Authenticator::Methods methods)
{
    m_state = Authenticating;
    m_challengeCode = challengeCode;

    QVariantMap feedbackData;
    const auto availability = this->availability(&feedbackData);
    switch (availability) {
    case AuthenticationNotRequired:
        if (methods & Authenticator::Confirmation) {
            qCDebug(daemon, "Authentication requested. Requesting simple confirmation.");
            startAuthentication(AuthenticationInput::Authorize, pid, QVariantMap(), Authenticator::Confirmation);
        } else {
            qCDebug(daemon, "Authentication requested. Unsecured, authenticating immediately.");
            authenticated(authenticateChallengeCode(
                              challengeCode,
                              Authenticator::NoAuthentication,
                              connectionPid(QDBusContext::connection())));
        }
        break;
    case CanAuthenticateSecurityCode:
        methods &= Authenticator::SecurityCode | Authenticator::Confirmation;
        // Fall through.
    case CanAuthenticate:
        qCDebug(daemon, "Authentication requested using methods %i.", int(methods));
        if (methods == Authenticator::Confirmation) {
            startAuthentication(AuthenticationInput::Authorize, pid, QVariantMap(), Authenticator::Confirmation);
        } else {
            startAuthentication(AuthenticationInput::EnterSecurityCode, pid, QVariantMap(), methods);
        }
        break;
    case SecurityCodeRequired:
        m_challengeCode.clear();
        authenticationUnavailable(AuthenticationInput::FunctionUnavailable);
        break;
    case CodeEntryLockedRecoverable:
    case CodeEntryLockedPermanent:
    case ManagerLockedRecoverable:
    case ManagerLockedPermanent:
        m_challengeCode.clear();
        lockedOut(availability, &HostAuthenticationInput::authenticationUnavailable, feedbackData);
        break;
    }
}


void HostAuthenticator::requestPermission(
        const QString &client,
        const QString &message,
        const QVariantMap &properties,
        Authenticator::Methods methods)
{
    const auto pid = connectionPid(QDBusContext::connection());

    cancelPending();

    if (m_state == Idle) {
        setActiveClient(client);
        beginRequestPermission(pid, message, properties, methods);
    } else {
        m_pending.connection = QDBusContext::connection().name();
        m_pending.client = client;
        m_pending.pid = pid;
        m_pending.request = PermissionRequest;
        m_pending.message = message;
        m_pending.properties = properties;
        m_pending.methods = methods;
        cancel();
    }
}

void HostAuthenticator::beginRequestPermission(
        uint pid, const QString &message, const QVariantMap &properties, Authenticator::Methods methods)
{
    m_state = RequestingPermission;

    const uint authenticatingPid = properties.value(
                QStringLiteral("authenticatingPid"), QVariant::fromValue(pid)).toUInt();

    QVariantMap data = {
        { QStringLiteral("message"), message }
    };

    const auto availability = this->availability(&data);
    switch (availability) {
    case AuthenticationNotRequired:
        qCDebug(daemon, "Authentication requested. Requesting simple confirmation.");
        startAuthentication(AuthenticationInput::Authorize, authenticatingPid, data, Authenticator::Confirmation);
        break;
    case CanAuthenticateSecurityCode:
        methods &= Authenticator::SecurityCode | Authenticator::Confirmation;
        // Fall through.
    case CanAuthenticate:
        qCDebug(daemon, "Authentication requested using methods %i.", int(methods));
        if (methods == Authenticator::Confirmation) {
            startAuthentication(AuthenticationInput::Authorize, authenticatingPid, data, methods);
        } else {
            startAuthentication(AuthenticationInput::EnterSecurityCode, authenticatingPid, data, methods);
        }
        break;
    case SecurityCodeRequired:
        authenticationUnavailable(AuthenticationInput::FunctionUnavailable, authenticatingPid);
        break;
    case CodeEntryLockedRecoverable:
    case CodeEntryLockedPermanent:
    case ManagerLockedRecoverable:
    case ManagerLockedPermanent:
        lockedOut(availability, &HostAuthenticationInput::authenticationUnavailable, data);
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

    cancelPending();

    if (m_state == Idle) {
        setActiveClient(client);
        beginChangeSecurityCode(pid, challengeCode);
    } else {
        m_pending.connection = QDBusContext::connection().name();
        m_pending.client = client;
        m_pending.pid = pid;
        m_pending.request = ChangeRequest;
        m_pending.challengeCode = challengeCode;
        cancel();
    }
}

void HostAuthenticator::beginChangeSecurityCode(uint pid, const QVariant &challengeCode)
{
    m_state = AuthenticatingForChange;
    m_challengeCode = challengeCode;

    switch (availability()) {
    case AuthenticationNotRequired:
    case SecurityCodeRequired:
        // The logic here should match enterCodeChangeState(). We can't call that directly though
        // because we need to pass the extra pid argument to startAuthentication().
        if (codeGeneration() == AuthenticationInput::MandatoryCodeGeneration) {
            m_state = ExpectingGeneratedSecurityCode;
            startAuthentication(AuthenticationInput::SuggestSecurityCode, pid, generatedCodeData(), Authenticator::SecurityCode);
        } else {
            m_state = EnteringNewSecurityCode;
            startAuthentication(AuthenticationInput::EnterNewSecurityCode, pid, QVariantMap(), Authenticator::SecurityCode);
        }
        break;
    case CanAuthenticateSecurityCode:
    case CanAuthenticate:
        startAuthentication(AuthenticationInput::EnterSecurityCode, pid, QVariantMap(), Authenticator::SecurityCode);
        break;
    case CodeEntryLockedRecoverable:
    case CodeEntryLockedPermanent:
    case ManagerLockedRecoverable:
    case ManagerLockedPermanent:
        m_challengeCode.clear();
        authenticationUnavailable(AuthenticationInput::FunctionUnavailable);
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

    cancelPending();

    if (m_state == Idle) {
        setActiveClient(client);
        beginClearSecurityCode(pid);
    } else {
        m_pending.connection = QDBusContext::connection().name();
        m_pending.client = client;
        m_pending.pid = pid;
        m_pending.request = ClearRequest;
        cancel();
    }
}

void HostAuthenticator::beginClearSecurityCode(uint pid)
{
    m_state = AuthenticatingForClear;

    switch (availability()) {
    case AuthenticationNotRequired:
        m_state = Idle;
        QDBusContext::sendErrorReply(QDBusError::InvalidArgs);
        break;
    case CanAuthenticateSecurityCode:
    case CanAuthenticate:
        startAuthentication(
                    AuthenticationInput::EnterSecurityCode, pid, QVariantMap(), Authenticator::SecurityCode);
        break;
    case SecurityCodeRequired:
    case CodeEntryLockedRecoverable:
    case CodeEntryLockedPermanent:
    case ManagerLockedRecoverable:
    case ManagerLockedPermanent:
        authenticationUnavailable(AuthenticationInput::FunctionUnavailable);
        break;
    }
}

void HostAuthenticator::enterSecurityCode(const QString &code)
{
    switch (m_state) {
    case Idle:
        return;
    case Authenticating:
        qCDebug(daemon, "Security code entered for authentication.");
        m_state = AuthenticationEvaluating;
        checkCodeFinished(checkCode(code));
        return;
    case RequestingPermission:
        qCDebug(daemon, "Security code entered for authentication.");
        m_state = PermissionEvaluating;
        checkCodeFinished(checkCode(code));
        return;
    case AuthenticatingForChange: {
        qCDebug(daemon, "Security code entered for code change authentication.");
        m_state = AuthenticationForChangeEvaluating;
        int result = checkCode(code);
        switch (result) {
        case Evaluating:
        case Success:
        case SecurityCodeExpired:
            m_currentCode = code;
            break;
        }
        checkCodeFinished(result);
        return;
    }
    case EnteringNewSecurityCode:
        qCDebug(daemon, "New security code entered.");
        m_newCode = code;
        m_state = RepeatingNewSecurityCode;
        m_repeatsRequired = 1;
        feedback(AuthenticationInput::RepeatNewSecurityCode, -1);
        return;
    case ExpectingGeneratedSecurityCode:
        if (m_generatedCode == code) {
            m_newCode = code;
            m_state = RepeatingNewSecurityCode;
            m_repeatsRequired = 2;
            feedback(AuthenticationInput::EnterNewSecurityCode, -1);
        } else {
            feedback(AuthenticationInput::SecurityCodesDoNotMatch, QVariantMap());
            feedback(AuthenticationInput::SuggestSecurityCode, generatedCodeData());
        }
        return;
    case RepeatingNewSecurityCode: {
        qCDebug(daemon, "New security code confirmation entered.");
        if (m_newCode != code) {
            qCDebug(daemon, "Lock codes don't match.");
            m_newCode.clear();

            feedback(AuthenticationInput::SecurityCodesDoNotMatch, -1);

            switch (availability()) {
            case AuthenticationNotRequired:
            case SecurityCodeRequired:
                enterCodeChangeState(&HostAuthenticationInput::feedback, Authenticator::SecurityCode);
                break;
            default:
                m_state = AuthenticatingForChange;
                feedback(AuthenticationInput::EnterSecurityCode, -1);
            }
        } else if (--m_repeatsRequired > 0) {
            feedback(AuthenticationInput::RepeatNewSecurityCode, -1);
        } else {
            m_newCode.clear();

            setCodeFinished(setCode(m_currentCode, code));
        }
        return;
    }
    case AuthenticatingForClear: {
        qCDebug(daemon, "Security code entered for clear authentication.");
        m_state = AuthenticationForClearEvaluating;
        int result = checkCode(code);
        switch (result) {
            case Evaluating:
            case Success:
                m_currentCode = code;
                break;
        }
        checkCodeFinished(result);
        return;
    }
    default:
        return;
    }
}

void HostAuthenticator::requestSecurityCode()
{
    if (m_state == EnteringNewSecurityCode
            && codeGeneration() != AuthenticationInput::NoCodeGeneration) {
        feedback(AuthenticationInput::EnterNewSecurityCode, generatedCodeData());
    } else if (m_state == ExpectingGeneratedSecurityCode) {
        feedback(AuthenticationInput::SuggestSecurityCode, generatedCodeData());
    }
}

void HostAuthenticator::authorize()
{
    switch (m_state) {
    case Authenticating:
    case AuthenticationEvaluating:
    case RequestingPermission:
    case PermissionEvaluating:
        if (activeMethods() == Authenticator::Confirmation) {
            qCDebug(daemon, "Action authorized without authentication.");
            confirmAuthentication(Authenticator::Confirmation);
        } else {
            qCWarning(daemon, "Authorization requires authentication, rejecting.");
        }
        break;
    default:
        break;
    }
}

void HostAuthenticator::checkCodeFinished(int result)
{
    const FeedbackFunction feebackFunction = (m_state & EvaluatingFlag)
        ? &HostAuthenticationInput::authenticationResumed
        : static_cast<void (HostAuthenticationInput::*)(AuthenticationInput::Feedback, const QVariantMap &, Authenticator::Methods)>(&HostAuthenticationInput::feedback);

    m_state = State(m_state & ~EvaluatingFlag);

    switch (m_state) {
    case Authenticating:
    case RequestingPermission:
        switch (result) {
        case Evaluating:
            authenticationEvaluating();
            m_state = State(m_state | EvaluatingFlag);
            return;
        case Success:
        case SecurityCodeExpired:
            confirmAuthentication(Authenticator::SecurityCode);
            return;
        case LockedOut:
            lockedOut();
            return;
        }
        break;
    case AuthenticationCanceled:
        if (result == Success || result == SecurityCodeExpired) {
            // If the check succeeded we send success even though the user canceled.
            // This is internally consistent with the behavior you'd see if we dispatched
            // a success condition over IPC in the interval between when the client sent a cancel
            // and we were able to process it.
            m_state = Authenticating;
            confirmAuthentication(Authenticator::SecurityCode);
        } else {
            aborted();
        }
        return;
    case AuthenticationCompleted:
        authenticationEnded(true);
        return;
    case AuthenticatingForChange:
        switch (result) {
        case Evaluating:
            authenticationEvaluating();
            m_state = State(m_state | EvaluatingFlag);
            return;
        case Success:
        case SecurityCodeExpired:
            enterCodeChangeState(feebackFunction, Authenticator::SecurityCode);
            return;
        case LockedOut:
            lockedOut();
            return;
        }
        break;
    case AuthenticationForChangeCanceled:
        securityCodeChangeAborted();
        return;
    case AuthenticatingForClear: {
        switch (result) {
        case Evaluating:
            authenticationEvaluating();
            m_state = State(m_state | EvaluatingFlag);
            return;
        case Success:
            if (clearCode(m_currentCode)) {
                securityCodeCleared();
            } else {
                abortAuthentication(AuthenticationInput::SoftwareError);
            }
            m_currentCode.clear();
            return;
        }
        break;
    }
    case AuthenticationForClearCanceled:
        securityCodeClearAborted();
        return;
    default:
        return;
    }

    const int attempts = result;
    const int maximum = maximumAttempts();

    QVariantMap data;
    if (maximum > 0 && attempts > 0) {
        data.insert(attemptsRemaining, qMax(0, maximum - attempts));
        (this->*feebackFunction)(AuthenticationInput::IncorrectSecurityCode, data, Authenticator::Methods());

        if (attempts >= maximum) {
            lockedOut();
            return;
        }
    } else {
        data.insert(attemptsRemaining, -1);
        (this->*feebackFunction)(AuthenticationInput::IncorrectSecurityCode, data, Authenticator::Methods());
    }
}

void HostAuthenticator::setCodeFinished(int result)
{
    switch (result) {
    case Success:
        m_currentCode.clear();

        qCDebug(daemon, "Security code changed.");
        securityCodeChanged(authenticateChallengeCode(
                    m_challengeCode, Authenticator::SecurityCode, m_authenticatingPid));
        break;
    case SecurityCodeInHistory:
        if (m_state == ChangeCanceled) {
            securityCodeChangeAborted();
        } else {
            qCDebug(daemon, "Security code disallowed.");
            feedback(AuthenticationInput::SecurityCodeInHistory, -1);
            if (m_state == Changing) {
                enterCodeChangeState(&HostAuthenticationInput::authenticationResumed, Authenticator::SecurityCode);
            } else {
                enterCodeChangeState(&HostAuthenticationInput::feedback, Authenticator::SecurityCode);
            }
        }
        break;
    case Evaluating:
        if (m_state == RepeatingNewSecurityCode) {
            m_state = Changing;
            authenticationEvaluating();
        } else {
            abortAuthentication(AuthenticationInput::SoftwareError);
        }
        break;
    default:
        m_currentCode.clear();
        qCDebug(daemon, "Security code change failed.");

        abortAuthentication(AuthenticationInput::SoftwareError);
        break;
    }
}

void HostAuthenticator::confirmAuthentication(Authenticator::Method method)
{
    switch (m_state) {
    case Authenticating:
        authenticated(authenticateChallengeCode(m_challengeCode, method, m_authenticatingPid));
        break;
    case AuthenticationEvaluating:
        sendToActiveClient(authenticatorInterface, QStringLiteral("Authenticated"), authenticateChallengeCode(m_challengeCode, method, m_authenticatingPid));
        m_state = AuthenticationCompleted;
        authenticationInactive();
        break;
    case RequestingPermission:
        sendToActiveClient(authenticatorInterface, QStringLiteral("PermissionGranted"), uint(method));
        authenticationEnded(true);
        break;
    case PermissionEvaluating:
        sendToActiveClient(authenticatorInterface, QStringLiteral("PermissionGranted"), uint(method));
        m_state = AuthenticationCompleted;
        authenticationInactive();
        break;
    default:
        break;
    }
}

void HostAuthenticator::abortAuthentication(AuthenticationInput::Error error)
{
    switch (m_state) {
    case Authenticating:
    case RequestingPermission:
        m_state = AuthenticationError;
        break;
    case AuthenticatingForChange:
    case EnteringNewSecurityCode:
    case RepeatingNewSecurityCode:
    case ExpectingGeneratedSecurityCode:
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

void HostAuthenticator::authenticationStarted(
        Authenticator::Methods methods, uint authenticatingPid, AuthenticationInput::Feedback feedback)
{
    m_authenticatingPid = authenticatingPid;

    HostAuthenticationInput::authenticationStarted(methods, authenticatingPid, feedback);
}

void HostAuthenticator::authenticationEnded(bool confirmed)
{
    clearActiveClient();

    m_authenticatingPid = 0;
    m_challengeCode.clear();
    m_state = Idle;
    m_currentCode.clear();
    m_newCode.clear();

    HostAuthenticationInput::authenticationEnded(confirmed);

    beginPending();
}

void HostAuthenticator::cancel()
{
    switch (m_state) {
    // Nothing to cancel.
    case Idle:
        return;
    // We're waiting for the user to provide some authentication credentials.
    case Authenticating:
    case RequestingPermission:
    // Something went wrong and we're waiting for the user to acknowledge the error and dismiss the
    // dialog.
    case AuthenticationError:
        aborted();
        return;
    // We're waiting for the user to enter their current or new security code as part of changing it.
    case AuthenticatingForChange:
    case EnteringNewSecurityCode:
    case RepeatingNewSecurityCode:
    case ExpectingGeneratedSecurityCode:
    case ChangeError:
        securityCodeChangeAborted();
        return;
    // We're waiting for the user to enter their current security code to disable device lock.
    case AuthenticatingForClear:
    case ClearError:
        securityCodeClearAborted();
        return;
    // We're waiting for the implementation to perform a time consuming and uninterruptable operation.
    // We can't interrupt so we make note of that so we can abort when that completes.
    case Changing:
        m_state = ChangeCanceled;
        return;
    case AuthenticationEvaluating:
    case PermissionEvaluating:
        m_state = AuthenticationCanceled;
        authenticationInactive();
        return;
    case AuthenticationForChangeEvaluating:
        m_state = AuthenticationForClearCanceled;
        authenticationInactive();
        return;
    case AuthenticationForClearEvaluating:
        m_state = AuthenticationForChangeCanceled;
        authenticationInactive();
        return;
    // Something has already tried to interrupt a time consuming and uninterruptable operation.
    case ChangeCanceled:
    case AuthenticationCanceled:
    case AuthenticationCompleted:
    case AuthenticationForChangeCanceled:
    case AuthenticationForClearCanceled:
        return;
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
    m_currentCode.clear();
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
    const QString connection = QDBusContext::connection().name();
    const QString address = QDBusContext::message().service();

    if (m_pending.request != NoRequest) {
        if (m_pending.connection == connection && m_pending.address == address && m_pending.client == client) {
            cancelPending();
        }
    } else if (isActiveClient(connection, address, client)) {
        cancel();
    }
}

void HostAuthenticator::cancelPending()
{
    switch (m_pending.request) {
    case NoRequest:
        break;
    case AuthenticateRequest:
    case PermissionRequest:
        NemoDBus::send(m_pending.connection, m_pending.client, authenticatorInterface, QStringLiteral("Aborted"));
        break;
    case ChangeRequest:
        NemoDBus::send(m_pending.connection, m_pending.client, securityCodeInterface, QStringLiteral("ChangeAborted"));
        break;
    case ClearRequest:
        NemoDBus::send(m_pending.connection, m_pending.client, securityCodeInterface, QStringLiteral("ClearAborted"));
        break;
    }
    m_pending.clear();
}

void HostAuthenticator::beginPending()
{
    // Take a temporary copy of the pending data and clear the current state.
    Pending pending;
    std::swap(pending, m_pending);

    switch (pending.request) {
    case NoRequest:
        break;
    case AuthenticateRequest:
        setActiveClient(pending.connection, pending.address, pending.client);
        beginAuthenticate(pending.pid, pending.challengeCode, pending.methods);
        break;
    case PermissionRequest:
        setActiveClient(pending.connection, pending.address, pending.client);
        beginAuthenticate(pending.pid, pending.challengeCode, pending.methods);
        break;
    case ChangeRequest:
        setActiveClient(pending.connection, pending.address, pending.client);
        beginAuthenticate(pending.pid, pending.challengeCode, pending.methods);
        break;
    case ClearRequest:
        setActiveClient(pending.connection, pending.address, pending.client);
        beginAuthenticate(pending.pid, pending.challengeCode, pending.methods);
        break;
    }
}

QVariantMap HostAuthenticator::generatedCodeData()
{
    m_generatedCode = generateCode();

    QVariantMap data;
    data.insert(QStringLiteral("securityCode"), m_generatedCode);
    return data;
}

void HostAuthenticator::enterCodeChangeState(FeedbackFunction feedback, Authenticator::Methods methods)
{
    if (codeGeneration() == AuthenticationInput::MandatoryCodeGeneration) {
        m_state = ExpectingGeneratedSecurityCode;
        (this->*feedback)(AuthenticationInput::SuggestSecurityCode, generatedCodeData(), methods);
    } else {
        m_state = EnteringNewSecurityCode;
        (this->*feedback)(AuthenticationInput::EnterNewSecurityCode, QVariantMap(), methods);
    }
}

void HostAuthenticator::Pending::clear()
{
    *this = Pending();
}

}
