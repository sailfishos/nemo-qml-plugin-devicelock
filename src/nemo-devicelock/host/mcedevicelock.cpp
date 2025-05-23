/*
 * Copyright (C) 2016 Jolla Ltd
 * Contact: Jonni Rainisto <jonni.rainisto@jollamobile.com>
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

#include "mcedevicelock.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFileInfo>

#include <sys/time.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>

namespace NemoDeviceLock
{

/** Maximum extra delay when waking up from suspend to apply devicelock */
#define DEVICELOCK_MAX_WAKEUP_DELAY_S 12

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qCInfo qCDebug
#endif

MceDeviceLock::MceDeviceLock(Authenticator::Methods allowedMethods, QObject *parent)
    : HostDeviceLock(allowedMethods, parent)
    , m_adaptor(this)
    , m_mceRequest(
          this,
          systemBus(),
          QStringLiteral(MCE_SERVICE),
          QStringLiteral(MCE_REQUEST_PATH),
          QStringLiteral(MCE_REQUEST_IF))
    , m_locked(false)
    , m_callActive(false)
    , m_displayOn(true)
    , m_tklockActive(true)
    , m_userActivity(true)
    , m_lpmMode(false)
{
    connect(&m_hbTimer, &BackgroundActivity::running, this, &MceDeviceLock::lock);

    trackMceProperty(
                QStringLiteral(MCE_CALL_STATE_SIG),
                SLOT(handleCallStateChanged(QString)),
                QStringLiteral(MCE_CALL_STATE_GET),
                &MceDeviceLock::handleCallStateChanged);

    trackMceProperty(
                QStringLiteral(MCE_DISPLAY_SIG),
                SLOT(handleDisplayStateChanged(QString)),
                QStringLiteral(MCE_DISPLAY_STATUS_GET),
                &MceDeviceLock::handleDisplayStateChanged);

    trackMceProperty(
                QStringLiteral(MCE_TKLOCK_MODE_SIG),
                SLOT(handleTklockStateChanged(QString)),
                QStringLiteral(MCE_TKLOCK_MODE_GET),
                &MceDeviceLock::handleTklockStateChanged);

    systemBus().connectToSignal(
                QString(),
                QStringLiteral(MCE_SIGNAL_PATH),
                QStringLiteral(MCE_SIGNAL_IF),
                QStringLiteral(MCE_INACTIVITY_SIG),
                this,
                SLOT(handleInactivityStateChanged(bool)));

    const auto response = m_mceRequest.call(QStringLiteral(MCE_INACTIVITY_STATUS_GET));
    response->onFinished<bool>([this](bool state) {
        handleInactivityStateChanged(state);
    });

    /* Note: LPM mode can't be queried at the time of writing */
    systemBus().connectToSignal(
                QString(),
                QStringLiteral(MCE_SIGNAL_PATH),
                QStringLiteral(MCE_SIGNAL_IF),
                QStringLiteral(MCE_LPM_UI_MODE_SIG),
                this,
                SLOT(handleLpmModeChanged(const QString &)));

    systemBus().registerObject(QStringLiteral("/devicelock"), this);
}

MceDeviceLock::~MceDeviceLock()
{
}

void MceDeviceLock::trackMceProperty(
        const QString &changedSignal,
        const char *changedSlot,
        const QString &getMethod,
        void (MceDeviceLock::*replySlot)(const QString &))
{
    systemBus().connectToSignal(
                QString(),
                QStringLiteral(MCE_SIGNAL_PATH),
                QStringLiteral(MCE_SIGNAL_IF),
                changedSignal,
                this,
                changedSlot);

    const auto response = m_mceRequest.call(getMethod);
    response->onFinished<QString>([this, replySlot](const QString &state) {
        (this->*replySlot)(state);
    });
}

/** Handle tklock state signal/reply from mce
 */
void MceDeviceLock::handleTklockStateChanged(const QString &state)
{
    bool active = (state == MCE_TK_LOCKED);

    if (m_tklockActive != active) {
        qCDebug(daemon, "MCE tklock state is now %s", qPrintable(state));

        m_tklockActive = active;
        setStateAndSetupLockTimer();
    }
}

/** Handle call state signal/reply from mce
 */
void MceDeviceLock::handleCallStateChanged(const QString &state)
{
    bool active = (state == MCE_CALL_STATE_ACTIVE || state == MCE_CALL_STATE_RINGING);

    if (m_callActive != active) {
        qCDebug(daemon, "MCE call state is now %s", qPrintable(state));

        m_callActive = active;
        setStateAndSetupLockTimer();
    }
}

/** Handle display state signal/reply from mce
 */
void MceDeviceLock::handleDisplayStateChanged(const QString &state)
{
    bool displayOn = (state == MCE_DISPLAY_ON_STRING || state == MCE_DISPLAY_DIM_STRING);

    if (m_displayOn != displayOn) {
        qCDebug(daemon, "MCE display state is now %s", qPrintable(state));

        m_displayOn = displayOn;
        setStateAndSetupLockTimer();
    }
}

/** Handle inactivity state signal/reply from mce
 */
void MceDeviceLock::handleInactivityStateChanged(const bool state)
{
    bool activity = !state;

    if (m_userActivity != activity) {
        qCDebug(daemon, "MCE inactivity state is now %s", activity ? "true" : "false");

        m_userActivity = activity;
        setStateAndSetupLockTimer();
    }
}

/** Handle LPM UI Mode signal from mce
 */
void MceDeviceLock::handleLpmModeChanged(const QString &state)
{
    bool lpmMode = (state == MCE_LPM_UI_ENABLED);

    if (m_lpmMode != lpmMode) {
        qCDebug(daemon, "MCE LPM mode is now %s", lpmMode ? "true" : "false");

        m_lpmMode = lpmMode;
        setStateAndSetupLockTimer();
    }
}

/** Helper for producing human readable devicelock state logging
 */
static const char *reprLockState(bool locked)
{
    return locked ? "Locked" : "Unlocked";
}

/** Evaluate initial devicelock state
 */
void MceDeviceLock::init()
{
    setLocked(automaticLocking() >= 0);
    stateChanged();
}

/** Evaluate devicelock state we should be in
 */
bool MceDeviceLock::getRequiredLockState()
{
    /* Assume current state is ok */
    bool locked = m_locked;

    if (automaticLocking() < 0) {
        /* Device locking is disabled */
        locked = false;
    } else if (automaticLocking() == 0 && !m_displayOn && !m_lpmMode && !m_callActive) {
        /* Display is off in immediate lock mode */
        locked = true;
    }

    return locked;
}

/** Check if devicelock timer should be running
 */
bool MceDeviceLock::needLockTimer()
{
    /* Must be currently unlocked */
    if (m_locked)
        return false;

    /* Must not be disabled or in lock-immediate mode */
    if (automaticLocking() <= 0)
        return false;

    /* Must not be in manual-only mode */
    if (automaticLocking() >= 254)
        return false;

    /* Must not have active call */
    if (m_callActive)
        return false;

    /* Must not be in active use */
    if (m_displayOn && !m_tklockActive && m_userActivity)
        return false;

    return true;
}

/** Evaluate required devicelock state and/or need for timer
 */
void MceDeviceLock::setStateAndSetupLockTimer()
{
    const bool requiredState = getRequiredLockState();

    if (m_locked != requiredState) {
        /* We should be in different deviceLockState. Set the state
         * and assume that setState() recurses back here so that we
         * get another chance to deal with the stable state. */
            qCDebug(daemon, "forcing %s instead of %s",
                        reprLockState(requiredState), reprLockState(m_locked));
        setLocked(requiredState);
    } else if (needLockTimer()) {
        /* Start devicelock timer */
        if (!m_hbTimer.isWaiting()) {
            int range_lo = automaticLocking() * 60;
            int range_hi = range_lo + DEVICELOCK_MAX_WAKEUP_DELAY_S;

            qCInfo(daemon, "start devicelock timer (%d-%d s)", range_lo, range_hi);

            m_hbTimer.wait(range_lo, range_hi);
        } else {
            qCDebug(daemon, "devicelock timer already running");
        }
    } else {
        /* Stop devicelock timer */
        if (!m_hbTimer.isStopped()) {
            qCInfo(daemon, "stop devicelock timer");

            m_hbTimer.stop();
        }
    }
}

/** Slot for locking device on timer trigger
 */
void MceDeviceLock::lock()
{
    qCInfo(daemon, "devicelock triggered");

    setLocked(true);

    /* The setState() call should end up terminating/restarting the
     * timer. If that does not happen, it is a bug. Nevertheless, we
     * must not leave an active cpu keepalive session behind. */
    if (m_hbTimer.isRunning()) {
        qCWarning(daemon, "cpu keepalive was not terminated; forcing stop");

        m_hbTimer.stop();
    }
}

bool MceDeviceLock::isLocked() const
{
    return m_locked;
}

/** Explicitly set devicelock state
 */
void MceDeviceLock::setLocked(bool locked)
{
    if (m_locked == locked) {
        return;
    }

    qCInfo(daemon, "%s -> %s", reprLockState(m_locked), reprLockState(locked));

    m_locked = locked;

    stateChanged();
}

void MceDeviceLock::stateChanged()
{
    setStateAndSetupLockTimer();

    emit HostDeviceLock::stateChanged();
    emit m_adaptor.stateChanged(state());
}

void MceDeviceLock::authenticationChecked(bool success)
{
    emit m_adaptor.authenticationChecked(success);
}

void MceDeviceLock::automaticLockingChanged()
{
    setStateAndSetupLockTimer();
}

MceDeviceLockAdaptor::MceDeviceLockAdaptor(MceDeviceLock *deviceLock)
    : QDBusAbstractAdaptor(deviceLock)
    , m_deviceLock(deviceLock)
{
    setAutoRelaySignals(true);
}

int MceDeviceLockAdaptor::state()
{
    return m_deviceLock->state();
}

void MceDeviceLockAdaptor::setState(int state)
{
    if (state != DeviceLock::Locked) {
        // Unauthenticated unlocking is not accepted.
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied);
    } else if (m_deviceLock->automaticLocking() == -1) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Device lock not in use"));
    } else {
        m_deviceLock->setLocked(true);
    }
}

void MceDeviceLockAdaptor::activateTemporaryLockout()
{
    if (m_deviceLock->automaticLocking() == -1) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Device lock not in use"));
        return;
    }

    uint callerServicePid(m_deviceLock->connection().interface()->servicePid(m_deviceLock->message().service()).value());
    QFileInfo info(QString("/proc/%1").arg(callerServicePid));
    bool callerIsRoot(info.owner() == "root");
    if (!callerIsRoot) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QString("PID %1 euid is not root").arg(callerServicePid));
        return;
    }

    qint64 temporaryLockTimeout = m_deviceLock->temporaryLockTimeout();
    if (temporaryLockTimeout <= 0) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Invalid temporaryLockTimeout setting"));
        return;
    }

    int maximumAttempts = m_deviceLock->maximumAttempts();
    if (maximumAttempts <= 0) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Invalid maximumAttempts setting"));
        return;
    }

    int currentAttempts = m_deviceLock->currentAttempts();
    if (currentAttempts < 0) {
        m_deviceLock->sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Invalid currentAttempts setting"));
        return;
    }

    if (currentAttempts != maximumAttempts)
        emit m_deviceLock->temporaryLockoutRequest();
}

}
