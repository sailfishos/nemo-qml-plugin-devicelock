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

AuthenticationInput::~AuthenticationInput()
{
    if (m_registered) {
        call(QStringLiteral("SetRegistered"), m_localPath, false);
    }
}

int AuthenticationInput::minimumCodeLength() const
{
    return m_settings->minimumLength;
}

int AuthenticationInput::maximumCodeLength() const
{
    return m_settings->maximumLength;
}
int AuthenticationInput::maximumAttempts() const
{
    return m_settings->maximumAttempts;
}

bool AuthenticationInput::codeInputIsKeyboard() const
{
    return m_settings->inputIsKeyboard;
}

Authenticator::Methods AuthenticationInput::utilizedMethods() const
{
    return m_utilizedMethods;
}

AuthenticationInput::Status AuthenticationInput::status() const
{
    return m_status;
}

int AuthenticationInput::authenticatingPid() const
{
    return m_authenticatingPid;
}

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
void AuthenticationInput::enterSecurityCode(const QString &code)
{
    call(QStringLiteral("EnterSecurityCode"), m_localPath, code);
}

void AuthenticationInput::cancel()
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Cancel authentication");

        call(QStringLiteral("Cancel"), m_localPath);
    }
}

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

void AuthenticationInput::handleAuthenticationEvaluating()
{
    if (m_status != Idle && m_status != Evaluating) {
        qCDebug(devicelock, "Authentication evaluating");

        m_status = Evaluating;

        emit authenticationEvaluating();
        emit statusChanged();
    }
}

void AuthenticationInput::handleAuthenticationEnded(bool confirmed)
{
    if (m_status != Idle) {
        qCDebug(devicelock, "Authentication ended.  Confirmed %s", confirmed ? "true" : "false");

        m_status = Idle;

        emit authenticationEnded(confirmed);
        emit statusChanged();
    }
}

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
