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

#include "cliauthenticator.h"

#include "lockcodewatcher.h"

#include <QDBusMessage>

namespace NemoDeviceLock
{

CliAuthenticator::CliAuthenticator(QObject *parent)
    : HostAuthenticator(Authenticator::LockCode, parent)
    , m_watcher(LockCodeWatcher::instance())
    , m_state(Idle)
{
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &CliAuthenticator::availableMethodsChanged);
    connect(m_watcher.data(), &LockCodeWatcher::lockCodeSetChanged,
            this, &CliAuthenticator::lockCodeSetChanged);
}

CliAuthenticator::~CliAuthenticator()
{
}

Authenticator::Methods CliAuthenticator::availableMethods() const
{
    Authenticator::Methods methods;

    if (m_watcher->lockCodeSet()) {
        methods |= Authenticator::LockCode;
    }

    return methods;
}

bool CliAuthenticator::isLockCodeSet() const
{
    return m_watcher->lockCodeSet();
}

void CliAuthenticator::authenticate(
        const QString &authenticator, const QVariant &, Authenticator::Methods methods)
{
    auto connection = QDBusContext::connection().name();

    cancel();

    if (m_watcher->lockCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = currentAttempts();

        m_authenticatorConnection = connection;
        m_authenticatorPath = authenticator;
        m_state = AuthenticationInput;

        if (maximum > 0 && attempts >= maximum) {
            authenticationUnavailable(AuthenticationInput::LockedOut);
        } else {
            authenticationStarted(methods, AuthenticationInput::EnterLockCode);
        }
    } else {
        // No code is set. Authenticate immediately with a dummy lock code.
        sendAuthenticated(connection, authenticator, QStringLiteral("12345"));
    }
}

void CliAuthenticator::changeLockCode(const QString &authenticator, const QVariant &)
{
    auto connection = QDBusContext::connection().name();

    cancel();

    m_authenticatorConnection = connection;
    m_authenticatorPath = authenticator;

    if (m_watcher->lockCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = currentAttempts();

        m_state = ChangeCurrentInput;

        if (maximum > 0 && attempts >= maximum) {
            authenticationUnavailable(AuthenticationInput::LockedOut);
        } else {
            authenticationStarted(Authenticator::LockCode, AuthenticationInput::EnterLockCode);
        }
    } else {
        m_state = ChangeNewInput;

        authenticationStarted(Authenticator::LockCode, AuthenticationInput::EnterNewLockCode);
    }
}

void CliAuthenticator::clearLockCode(const QString &authenticator)
{
    auto connection = QDBusContext::connection().name();

    cancel();

    if (m_watcher->lockCodeSet()) {
        const int maximum = maximumAttempts();
        const int attempts = currentAttempts();

        m_authenticatorConnection = connection;
        m_authenticatorPath = authenticator;
        m_state = AuthenticationInput;

        if (maximum > 0 && attempts >= maximum) {
            authenticationUnavailable(AuthenticationInput::LockedOut);
        } else {
            authenticationStarted(Authenticator::LockCode, AuthenticationInput::EnterLockCode);
        }
    } else {
        QDBusContext::sendErrorReply(QDBusError::InvalidArgs);
    }
}


void CliAuthenticator::enterLockCode(const QString &code)
{
    const auto connection = m_authenticatorConnection;
    const auto authenticator = m_authenticatorPath;

    PluginCommand *command = nullptr;

    switch (m_state) {
    case Idle:
        return;
    case AuthenticationInput:
        if ((command = m_watcher->checkCode(this, code))) {
            command->onSuccess([this, connection, authenticator, code]() {
                sendAuthenticated(connection, authenticator, code);
                complete();
            });
        } else {
            sendError(AuthenticationInput::SoftwareError);
            m_state = AuthenticationError;
            return;
        }
        break;
    case ChangeCurrentInput:
        if ((command = m_watcher->checkCode(this, code))) {
            command->onSuccess([this, connection, authenticator, code]() {
                m_state = ChangeNewInput;
                sendFeedback(AuthenticationInput::EnterNewLockCode, -1);
            });
        } else {
            sendError(AuthenticationInput::SoftwareError);
            m_state = ChangeError;
            return;
        }
        break;
    case ChangeNewInput:
        m_newCode = code;
        m_state = ChangeRepeatInput;
        sendFeedback(AuthenticationInput::RepeatNewLockCode, -1);
        return;
    case ChangeRepeatInput: {
        if (m_newCode != code) {
            m_currentCode.clear();
            m_newCode.clear();


            sendFeedback(AuthenticationInput::LockCodesDoNotMatch, -1);

            if (m_watcher->lockCodeSet()) {
                m_state = ChangeCurrentInput;
                sendFeedback(AuthenticationInput::EnterLockCode, -1);
            } else {
                m_state = ChangeNewInput;
                sendFeedback(AuthenticationInput::EnterNewLockCode, -1);
            }

            return;
        }

        const auto currentCode = m_currentCode;
        m_currentCode.clear();
        m_newCode.clear();

        if (const auto command = m_watcher->runPlugin(
                    this, QStringList() << QStringLiteral("--set-code") << currentCode << code)) {
            command->onSuccess([this, connection, authenticator, code]() {
                sendLockCodeChanged(connection, authenticator, code);
                complete();
            });
            command->onFailure([this, connection, authenticator](int) {
                sendError(AuthenticationInput::SoftwareError);
                m_state = ChangeError;
            });
            command->waitForFinished();
        } else {
            sendError(AuthenticationInput::SoftwareError);
            m_state = ChangeError;
        }
        return;
    }
    case ClearInput: {
        if ((command = m_watcher->runPlugin(
                 this, QStringList() << QStringLiteral("--clear-code") << code))) {
            command->onSuccess([this, connection, authenticator, code]() {
                sendLockCodeCleared(connection, authenticator);
                complete();
            });
        } else {
            sendError(AuthenticationInput::SoftwareError);
            m_state = ClearError;
            return;
        }
        break;
    }
    default:
        return;
    }

    Q_ASSERT(command);

    command->onFailure([this, connection, authenticator](int exitCode) {
        const int maximum = maximumAttempts();

        if (maximum > 0 && exitCode < 0) {
            const int attempts = -exitCode;
            sendFeedback(AuthenticationInput::IncorrectLockCode, qMax(0, maximum - attempts));

            if (attempts >= maximum) {
                sendError(AuthenticationInput::LockedOut);

                switch (m_state) {
                case AuthenticationInput:
                    m_state = AuthenticationError;
                    break;
                case ChangeCurrentInput:
                case ChangeNewInput:
                case ChangeRepeatInput:
                    m_state = ChangeError;
                    break;
                case ClearInput:
                    m_state = ClearError;
                    break;
                default:
                    break;
                }
                return;
            }
        } else {
            sendFeedback(AuthenticationInput::IncorrectLockCode, -1);
        }
    });
    command->waitForFinished();
}

void CliAuthenticator::complete()
{
    clearConnection();

    m_state = Idle;
    m_currentCode.clear();
    m_newCode.clear();

    authenticationEnded(true);
}

void CliAuthenticator::cancel()
{
    if (m_state != Idle) {
        if (!m_authenticatorConnection.isEmpty()) {
            switch (m_state) {
            case AuthenticationInput:
            case AuthenticationError:
                sendAborted(m_authenticatorConnection, m_authenticatorPath);
                break;
            case ChangeCurrentInput:
            case ChangeNewInput:
            case ChangeRepeatInput:
            case ChangeError:
                sendLockCodeChangeAborted(m_authenticatorConnection, m_authenticatorPath);
                break;
            case ClearInput:
            case ClearError:
                sendLockCodeClearAborted(m_authenticatorConnection, m_authenticatorPath);
                break;
            default:
                break;
            }
        }

        clearConnection();

        m_state = Idle;
        m_currentCode.clear();
        m_newCode.clear();
        authenticationEnded(false);
    }
}

void CliAuthenticator::cancel(const QString &authenticator)
{
    if (checkConnection(QDBusContext::connection().name(), authenticator)) {
        clearConnection();

        cancel();
    }
}

void CliAuthenticator::clientDisconnected(const QString &connectionName)
{
    if (m_authenticatorConnection == connectionName) {
        clearConnection();

        cancel();
    }

    HostAuthenticator::clientDisconnected(connectionName);
}

bool CliAuthenticator::checkConnection(const QString &connection, const QString &path)
{
    return m_authenticatorConnection == connection && m_authenticatorPath == path;
}

void CliAuthenticator::clearConnection()
{
    m_authenticatorConnection.clear();
    m_authenticatorPath.clear();
}

}
