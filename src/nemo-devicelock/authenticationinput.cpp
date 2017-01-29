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

#include "authenticationinput.h"
#include "settingswatcher.h"

#include "logging.h"

namespace NemoDeviceLock
{

AuthenticationInputAdaptor::AuthenticationInputAdaptor(AuthenticationInput *authenticationInput)
    : QDBusAbstractAdaptor(authenticationInput)
    , m_authenticationInput(authenticationInput)
{
}

void AuthenticationInputAdaptor::AuthenticationStarted(uint pid, uint utilizedMethods, uint instruction)
{
    m_authenticationInput->handleAuthenticationStarted(
                pid,
                Authenticator::Methods(utilizedMethods),
                AuthenticationInput::Feedback(instruction));
}

void AuthenticationInputAdaptor::AuthenticationUnavailable(uint pid, uint error)
{
    m_authenticationInput->handleAuthenticationUnavailable(
                pid,
                AuthenticationInput::Error(error));
}

void AuthenticationInputAdaptor::AuthenticationResumed(uint utilizedMethods, uint instruction)
{
    m_authenticationInput->handleAuthenticationResumed(
                Authenticator::Methods(utilizedMethods),
                AuthenticationInput::Feedback(instruction));
}

void AuthenticationInputAdaptor::AuthenticationEvaluating()
{
    m_authenticationInput->handleAuthenticationEvaluating();
}

void AuthenticationInputAdaptor::AuthenticationProgress(int current, int maximum)
{
    m_authenticationInput->authenticationProgress(current, maximum);
}

void AuthenticationInputAdaptor::AuthenticationEnded(bool confirmed)
{
    m_authenticationInput->handleAuthenticationEnded(confirmed);
}

void AuthenticationInputAdaptor::Feedback(uint feedback, uint attemptsRemaining, uint utilizedMethods)
{
    m_authenticationInput->handleFeedback(
                AuthenticationInput::Feedback(feedback),
                attemptsRemaining,
                Authenticator::Methods(utilizedMethods));
}

void AuthenticationInputAdaptor::Error(uint error)
{
    m_authenticationInput->handleError(AuthenticationInput::Error(error));
}

/*!
    \class AuthenticationInput
    \brief The AuthenticationInput class provides an interface between the security daemon and a security code input field.

    A security code input field is provided by a trusted process which is able to display a dialog
    prompting the user to enter their security code when an untrusted process requires user
    authentication for an action.

    The trusted process will create an instance of AuthenticationInput and register it with
    the security daemon when it can provide the dialog.  If another application requires
    authentication the authenticationStarted() signal will be emitted signaling that the
    input field should be displayed.  If an application requests authentication but an error
    prevents that the authenticationUnavailable() signal will instead be emitted prompting the
    input field to still be raised but displaying a description of the error.  The input field
    should remain visible until the authenticationEnded() signal is received.

    While an input field has focus and the user can interact with it it should set the active
    property to true, this informs the security daemon that other authentication methods such
    as fingerprint scanning should now also accept input.
*/

/*!
    Constructs an authentication input which handles requests of a given \a type as a child of
    \a parent.
*/
AuthenticationInput::AuthenticationInput(Type type, QObject *parent)
    : QObject(parent)
    , ConnectionClient(
        this,
        type == Authentication
            ? QStringLiteral("/authenticator")
            : QStringLiteral("/devicelock/lock"),
        QStringLiteral("org.nemomobile.devicelock.AuthenticationInput"))
    , m_adaptor(this)
    , m_settings(SettingsWatcher::instance())
    , m_utilizedMethods()
    , m_authenticatingPid(0)
    , m_status(Idle)
    , m_registered(false)
    , m_active(false)
{
    connect(m_settings.data(), &SettingsWatcher::maximumAttemptsChanged,
            this, &AuthenticationInput::maximumAttemptsChanged);
    connect(m_settings.data(), &SettingsWatcher::inputIsKeyboardChanged,
            this, &AuthenticationInput::codeInputIsKeyboardChanged);

    m_connection->onConnected(this, [this] {
        connected();

        if (m_registered) {
            call(QStringLiteral("SetRegistered"), m_localPath, true);
        }

        if (m_status != Idle) {
            m_status = Idle;

            emit statusChanged();
        }
    });

    m_connection->onDisconnected(this, [this] {
        handleError(SoftwareError);
    });

    if (m_connection->isConnected()) {
        connected();
    }
}

/*!
    Destroys an authentication input.
*/
AuthenticationInput::~AuthenticationInput()
{
    if (m_registered) {
        call(QStringLiteral("SetRegistered"), m_localPath, false);
    }
}

/*!
    \property NemoDeviceLock::AuthenticationInput::minimumCodeLength

    This property holds the minimum acceptable length of a security code.
*/

int AuthenticationInput::minimumCodeLength() const
{
    return m_settings->minimumLength;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::maximumCodeLength

    This property holds the maximum acceptable length of a security code.
*/

int AuthenticationInput::maximumCodeLength() const
{
    return m_settings->maximumLength;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::maximumAttempts

    This property holds the number of times the user may enter an incorrect security code before
    they are locked out.
*/

int AuthenticationInput::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::codeInputIsKeyboard

    This property holds whether the security code should be entered using a full keyboard instead
    of a simple numeric keyboard.
*/

bool AuthenticationInput::codeInputIsKeyboard() const
{
    return m_settings->inputIsKeyboard;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::utilizedMethods

    This property holds the set of authentication methods that are currently active.
*/

Authenticator::Methods AuthenticationInput::utilizedMethods() const
{
    return m_utilizedMethods;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::status

    This property holds the current status of the authentication input.
*/

AuthenticationInput::Status AuthenticationInput::status() const
{
    return m_status;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::authenticatingPid

    This property holds the PID of the application that is currently requesting authentication.
*/

int AuthenticationInput::authenticatingPid() const
{
    return m_authenticatingPid;
}

/*!
    \property NemoDeviceLock::AuthenticationInput::active

    This property holds whether the authentication input currently has focus.

    When this is set to true any other authentication methods provided by the security daemon
    will also accept input.
*/

bool AuthenticationInput::isActive() const
{
    return m_active;
}

void AuthenticationInput::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;

        if (m_status != Idle) {
            call(QStringLiteral("SetActive"), m_localPath, active);
        }

        emit activeChanged();
    }
}

/*!
    \property NemoDeviceLock::AuthenticationInput::registered

    This property holds whether the authentication input wishes to currently be registered with
    to handle authentication requests.

    The last input of a type to register with the security daemon will handle all authentication
    requests.  This allows trusted applications to override the default input and handle input
    internally.
*/

bool AuthenticationInput::isRegistered() const
{
    return m_registered;
}

void AuthenticationInput::setRegistered(bool registered)
{
    if (m_registered != registered) {
        m_registered = registered;

        call(QStringLiteral("SetRegistered"), m_localPath, registered);

        emit registeredChanged();
    }
}

/*!
    Sends an entered security \a code to the security daemon to be verified.
*/

void AuthenticationInput::enterSecurityCode(const QString &code)
{
    call(QStringLiteral("EnterSecurityCode"), m_localPath, code);
}

/*!
    Sends a request to cancel authentication to the security daemon.
*/

void AuthenticationInput::cancel()
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Cancel authentication");

        call(QStringLiteral("Cancel"), m_localPath);
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::authenticationStarted(Feedback feedback)

    Signals that an application has requested authentication and that the input should be displayed
    with the given initial \a feedback message.
*/

void AuthenticationInput::handleAuthenticationStarted(
        int pid, Authenticator::Methods utilizedMethods, Feedback feedback)
{
    qCDebug(devicelock, "Authentication started.  Methods: %i, Feedback: %i.",
            int(utilizedMethods), int(feedback));

    if (m_active) {
        call(QStringLiteral("SetActive"), m_localPath, true);
    }

    const auto previousStatus = m_status;
    const auto previousPid = m_authenticatingPid;
    const auto previousMethods = m_utilizedMethods;

    m_status = Authenticating;
    m_authenticatingPid = pid;
    m_utilizedMethods = utilizedMethods;

    if (m_authenticatingPid != previousPid) {
        emit authenticatingPidChanged();
    }

    if (m_utilizedMethods != previousMethods) {
        emit utilizedMethodsChanged();
    }

    emit authenticationStarted(feedback);

    if (m_status != previousStatus) {
        emit statusChanged();
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::authenticationUnavailable(Error error)

    Signals that the an application has requested authentication but it cannot be granted and
    the input should display the given \a error message.
*/

void AuthenticationInput::handleAuthenticationUnavailable(int pid, Error error)
{
    qCDebug(devicelock, "Authentication unavailable.  Error: %i.", int(error));

    const auto previousStatus = m_status;
    const auto previousPid = m_authenticatingPid;
    const auto previousMethods = m_utilizedMethods;

    m_status = AuthenticationError;
    m_authenticatingPid = pid;
    m_utilizedMethods = Authenticator::Methods();

    if (m_authenticatingPid != previousPid) {
        emit authenticatingPidChanged();
    }

    if (m_utilizedMethods != previousMethods) {
        emit utilizedMethodsChanged();
    }

    emit authenticationUnavailable(error);

    if (m_status != previousStatus) {
        emit statusChanged();
    }
}


void AuthenticationInput::handleAuthenticationResumed(
        Authenticator::Methods utilizedMethods, Feedback feedback)
{
    const auto previousStatus = m_status;
    const auto previousMethods = m_utilizedMethods;

    m_status = Authenticating;
    m_utilizedMethods = utilizedMethods;

    if (m_utilizedMethods != previousMethods) {
        emit utilizedMethodsChanged();
    }

    emit AuthenticationInput::feedback(feedback, -1);

    if (m_status != previousStatus) {
        emit statusChanged();
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::authenticationEvaluating()

    Signals that the security daemon is performing a potentially time consuming task during
    authentication.
*/

void AuthenticationInput::handleAuthenticationEvaluating()
{
    if (m_status != Idle && m_status != Evaluating) {
        qCDebug(devicelock, "Authentication evaluating");

        m_status = Evaluating;

        emit authenticationEvaluating();
        emit statusChanged();
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::authenticationEnded(bool confirmed)

    Signals that authentication has ended and the input can be hidden.  If the authentication
    was successful \a confirmed will be true.
*/

void AuthenticationInput::handleAuthenticationEnded(bool confirmed)
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Authentication ended.  Confirmed %s", confirmed ? "true" : "false");

        m_status = Idle;

        emit authenticationEnded(confirmed);
        emit statusChanged();
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::feedback(Feedback feedback, int attemptsRemaining)

    Signals that a \a feedback message should be shown to the user incorporating
    \a attemptsRemaining if it is not equal to -1.
*/

void AuthenticationInput::handleFeedback(
        Feedback feedback, int attemptsRemaining, Authenticator::Methods utilizedMethods)
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Authentication feedback %i.  Attempts remaining: %i. Methods: %i",
                    int(feedback), attemptsRemaining, int(utilizedMethods));

        const bool methodsChanged = m_utilizedMethods != utilizedMethods;

        m_utilizedMethods = utilizedMethods;

        emit AuthenticationInput::feedback(feedback, attemptsRemaining);

        if (methodsChanged) {
            emit utilizedMethodsChanged();
        }
    }
}

/*!
    \signal NemoDeviceLock::AuthenticationInput::error(Error error)

    Signals that an authentication \a error occured and a message should be shown to the user.

    After this the user will not be able to enter another security code until another authentication
    request is made.
*/

void AuthenticationInput::handleError(Error error)
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Authentication error %i.", int(error));

        const auto previousStatus = m_status;
        m_status = AuthenticationError;

        emit AuthenticationInput::error(error);

        if (m_status != previousStatus) {
            emit statusChanged();
        }
    }
}

void AuthenticationInput::connected()
{
    registerObject();
}

}
