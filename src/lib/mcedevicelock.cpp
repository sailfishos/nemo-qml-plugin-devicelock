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

#include "settingswatcher.h"

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

/** Maximum extra delay when waking up from suspend to apply devicelock */
#define DEVICELOCK_MAX_WAKEUP_DELAY_S 12

MceDeviceLock::MceDeviceLock(QObject *parent)
    : DeviceLock(parent)
    , m_dbus(this)
    , m_settings(SettingsWatcher::instance())
    , m_deviceLockState(Undefined)
    , m_callActive(false)
    , m_displayOn(true)
    , m_tklockActive(true)
    , m_userActivity(true)
    , m_verbosityLevel(1)
{
    connect(&m_hbTimer, &BackgroundActivity::running, this, &MceDeviceLock::lock);

    connect(this, &DeviceLock::automaticLockingChanged, this, &MceDeviceLock::setStateAndSetupLockTimer);

    trackCallState();
    trackDisplayState();
    trackTklockState();
    trackInactivityState();

    if (!QDBusConnection::systemBus().registerObject(QStringLiteral("/devicelock"), this)) {
        qWarning("Unable to register object at path /devicelock: %s",
                    QDBusConnection::systemBus().lastError().message().toUtf8().constData());
    }
}

MceDeviceLock::~MceDeviceLock()
{
}

/** Handle tklock state signal/reply from mce
 */
void MceDeviceLock::handleTklockStateChanged(const QString &state)
{
    bool active = (state == MCE_TK_LOCKED);

    if (m_tklockActive != active) {
        if (m_verbosityLevel >= 2)
            qDebug() << state;
        m_tklockActive = active;
        setStateAndSetupLockTimer();
    }
}

void MceDeviceLock::handleTklockStateReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        qCritical() << "Call to mce failed:" << reply.error();
    } else {
        handleTklockStateChanged(reply.value());
    }
    call->deleteLater();
}

static void trackMceProperty(
        MceDeviceLock *receiver,
        const QString &changedSignal,
        const char *changedSlot,
        const QString &getMethod,
        void (MceDeviceLock::*replySlot)(QDBusPendingCallWatcher *))
{
    QDBusConnection::systemBus().connect(
                QString(),
                QStringLiteral(MCE_SIGNAL_PATH),
                QStringLiteral(MCE_SIGNAL_IF),
                changedSignal,
                receiver,
                changedSlot);

    QDBusMessage call = QDBusMessage::createMethodCall(
                QStringLiteral(MCE_SERVICE),
                QStringLiteral(MCE_REQUEST_PATH),
                QStringLiteral(MCE_REQUEST_IF),
                getMethod);
    QDBusPendingCall reply = QDBusConnection::systemBus().asyncCall(call);
    QDBusPendingCallWatcher *watch = new QDBusPendingCallWatcher(reply, receiver);
    QObject::connect(watch, &QDBusPendingCallWatcher::finished, receiver, replySlot);
}

void MceDeviceLock::trackTklockState()
{
    trackMceProperty(
                this,
                QStringLiteral(MCE_TKLOCK_MODE_SIG),
                SLOT(handleTklockStateChanged(QString)),
                QStringLiteral(MCE_TKLOCK_MODE_GET),
                &MceDeviceLock::handleTklockStateReply);
}

/** Handle call state signal/reply from mce
 */
void MceDeviceLock::handleCallStateChanged(const QString &state)
{
    bool active = (state == MCE_CALL_STATE_ACTIVE || state == MCE_CALL_STATE_RINGING);

    if (m_callActive != active) {
        if (m_verbosityLevel >= 2)
            qDebug() << state;
        m_callActive = active;
        setStateAndSetupLockTimer();
    }
}

void MceDeviceLock::handleCallStateReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        qCritical() << "Call to mce failed:" << reply.error();
    } else {
        handleCallStateChanged(reply.value());
    }
    call->deleteLater();
}

void MceDeviceLock::trackCallState()
{
    trackMceProperty(
                this,
                QStringLiteral(MCE_CALL_STATE_SIG),
                SLOT(handleCallStateChanged(QString)),
                QStringLiteral(MCE_CALL_STATE_GET),
                &MceDeviceLock::handleCallStateReply);
}

/** Handle display state signal/reply from mce
 */
void MceDeviceLock::handleDisplayStateChanged(const QString &state)
{
    bool displayOn = (state == MCE_DISPLAY_ON_STRING || state == MCE_DISPLAY_DIM_STRING);

    if (m_displayOn != displayOn) {
        if (m_verbosityLevel >= 2) {
            qDebug() << state;
        }
        m_displayOn = displayOn;
        setStateAndSetupLockTimer();
    }
}

void MceDeviceLock::handleDisplayStateReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString> reply = *call;
    if (reply.isError()) {
        qCritical() << "Call to mce failed:" << reply.error();
    } else {
        handleDisplayStateChanged(reply.value());
    }
    call->deleteLater();
}

void MceDeviceLock::trackDisplayState()
{
    trackMceProperty(
                this,
                QStringLiteral(MCE_DISPLAY_SIG),
                SLOT(handleDisplayStateChanged(QString)),
                QStringLiteral(MCE_DISPLAY_STATUS_GET),
                &MceDeviceLock::handleDisplayStateReply);
}

/** Handle inactivity state signal/reply from mce
 */
void MceDeviceLock::handleInactivityStateChanged(const bool state)
{
    bool activity = !state;

    if (m_userActivity != activity) {
        if (m_verbosityLevel >= 2) {
            qDebug() << state;
        }
        m_userActivity = activity;
        setStateAndSetupLockTimer();
    }
}

void MceDeviceLock::handleInactivityStateReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<bool> reply = *call;
    if (reply.isError()) {
        qCritical() << "Call to mce failed:" << reply.error();
    } else {
        handleInactivityStateChanged(reply.value());
    }
    call->deleteLater();
}

void MceDeviceLock::trackInactivityState(void)
{
    trackMceProperty(
                this,
                QStringLiteral(MCE_INACTIVITY_SIG),
                SLOT(handleInactivityStateChanged(bool)),
                QStringLiteral(MCE_INACTIVITY_STATUS_GET),
                &MceDeviceLock::handleInactivityStateReply);
}

/** Helper for producing human readable devicelock state logging
 */
static const char *reprLockState(int state)
{
    switch (state) {
    case MceDeviceLock::Unlocked:  return "Unlocked";
    case MceDeviceLock::Locked:    return "Locked";
    case MceDeviceLock::Undefined: return "Undefined";
    default: break;
    }
    return "Invalid";
}

/** Evaluate initial devicelock state
 */
void MceDeviceLock::init()
{
    setState(automaticLocking() < 0 ? Unlocked : Locked);
}

/** Evaluate devicelock state we should be in
 */
MceDeviceLock::LockState MceDeviceLock::getRequiredLockState()
{
    /* Assume current state is ok */
    LockState requiredState = m_deviceLockState;

    if (m_deviceLockState == Undefined) {
        /* Initial state must be decided by init() */
    } else if (automaticLocking() < 0) {
        /* Device locking is disabled */
        requiredState = Unlocked;
    } else if (automaticLocking() == 0 && !m_displayOn) {
        /* Display is off in immediate lock mode */
        requiredState = Locked;
    }

    return requiredState;
}

/** Check if devicelock timer should be running
 */
bool MceDeviceLock::needLockTimer()
{
    /* Must be currently unlocked */
    if (m_deviceLockState != Unlocked)
        return false;

    /* Must not be disabled or in lock-immediate mode */
    if (automaticLocking() <= 0)
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
    LockState requiredState = getRequiredLockState();

    if (m_deviceLockState != requiredState) {
        /* We should be in different deviceLockState. Set the state
         * and assume that setState() recurses back here so that we
         * get another chance to deal with the stable state. */
        if (m_verbosityLevel >= 2)
            qDebug("forcing %s instead of %s",
                   reprLockState(requiredState),
                   reprLockState(m_deviceLockState));
        setState(requiredState);
    } else if (needLockTimer()) {
        /* Start devicelock timer */
        if (!m_hbTimer.isWaiting()) {
            int range_lo = automaticLocking() * 60;
            int range_hi = range_lo + DEVICELOCK_MAX_WAKEUP_DELAY_S;
            if (m_verbosityLevel >= 1)
                qDebug("start devicelock timer (%d-%d s)", range_lo, range_hi);
            m_hbTimer.wait(range_lo, range_hi);
        } else {
            if (m_verbosityLevel >= 2)
                qDebug("devicelock timer already running");
        }
    } else {
        /* Stop devicelock timer */
        if (!m_hbTimer.isStopped()) {
            if (m_verbosityLevel >= 1)
                qDebug("stop devicelock timer");
            m_hbTimer.stop();
        }
    }
}

/** Slot for locking device on timer trigger
 */
void MceDeviceLock::lock()
{
    if (m_verbosityLevel >= 1)
        qDebug() << "devicelock triggered";

    setState(Locked);

    /* The setState() call should end up terminating/restarting the
     * timer. If that does not happen, it is a bug. Nevertheless, we
     * must not leave an active cpu keepalive session behind. */
    if (m_hbTimer.isRunning()) {
        qWarning("cpu keepalive was not terminated; forcing stop");
        m_hbTimer.stop();
    }

}

DeviceLock::LockState MceDeviceLock::state() const
{
    return m_deviceLockState;
}

/** Explicitly set devicelock state
 */
void MceDeviceLock::setState(LockState state)
{
    if (m_deviceLockState == state) {
        return;
    }

    QString error;

    switch (state) {
    case Locked:
        if (automaticLocking() == -1) {
            error = QStringLiteral("Device lock not in use");
        }
        break;
    case Unlocked:
        if (!isPrivileged()) {
            error = QStringLiteral("Caller is not in privileged group");
        }
        break;
    case Undefined:
        /* Allow unit tests to rewind the state back to Undefined */
        if (!calledFromDBus()) {
            break;
        }
        /* Fall through */
    default:
        error = "Illegal state requested";
        break;
    }

    if (!error.isEmpty()) {
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::AccessDenied, error);
        }
        return;
    }

    if (m_verbosityLevel >= 1) {
        qDebug("%s -> %s",
               reprLockState(m_deviceLockState),
               reprLockState(state));
    }

    const bool isUnlock = m_deviceLockState == Locked && state == Unlocked;
    const bool isLock = m_deviceLockState ==  Unlocked && state == Locked;

    m_deviceLockState = state;
    emit m_dbus.stateChanged(m_deviceLockState);
    emit stateChanged();

    if (isUnlock) {
        emit unlocked();
    } else if (isLock) {
        emit locked();
    }

    setStateAndSetupLockTimer();
}

bool MceDeviceLock::isPrivileged()
{
    pid_t pid = -1;
    if (!calledFromDBus()) {
        // Local function calls are always privileged
        return true;
    }
    // Get the PID of the calling process
    pid = connection().interface()->servicePid(message().service());
    // The /proc/<pid> directory is owned by EUID:EGID of the process
    const QFileInfo info(QStringLiteral("/proc/%1").arg(pid));
    if (info.group() != QStringLiteral("privileged")
            && info.group() != QStringLiteral("disk")
            && info.owner() != QStringLiteral("root")) {
        return false;
    }
    return true;
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
    m_deviceLock->setState(DeviceLock::LockState(state));
}
