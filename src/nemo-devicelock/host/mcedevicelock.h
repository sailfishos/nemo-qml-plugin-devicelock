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

#ifndef NEMODEVICELOCK_MCEDEVICELOCK
#define NEMODEVICELOCK_MCEDEVICELOCK

#include <nemo-devicelock/host/hostdevicelock.h>

#include <sys/time.h>
#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QDBusPendingCallWatcher>
#include <keepalive/backgroundactivity.h>
#include <nemo-dbus/interface.h>

namespace NemoDeviceLock
{

class MceDeviceLock;

class MceDeviceLockAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.lipstick.devicelock")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.nemomobile.lipstick.devicelock\">\n"
"    <method name=\"state\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"state\"/>\n"
"    </method>\n"
"    <method name=\"setState\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"state\"/>\n"
"    </method>\n"
"    <method name=\"activateTemporaryLockout\">\n"
"    </method>\n"
"    <signal name=\"stateChanged\">\n"
"      <arg type=\"i\" name=\"state\"/>\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    explicit MceDeviceLockAdaptor(MceDeviceLock *deviceLock);

public slots:
    int state();
    void setState(int state);
    void activateTemporaryLockout();
signals:
    void stateChanged(int state);

private:
    MceDeviceLock * const m_deviceLock;
};

class MceDeviceLock : public HostDeviceLock
{
    Q_OBJECT
public:
    explicit MceDeviceLock(Authenticator::Methods allowedMethods, QObject *parent = nullptr);
    ~MceDeviceLock();

    bool isLocked() const override;
    void setLocked(bool locked) override;

protected:
    void init();
    void automaticLockingChanged() override;
    void stateChanged() override;

protected slots:
    void lock();

    void handleTklockStateChanged(const QString &state);
    void handleCallStateChanged(const QString &state);
    void handleDisplayStateChanged(const QString &state);
    void handleInactivityStateChanged(const bool state);
    void handleLpmModeChanged(const QString &state);

signals:
    void temporaryLockoutRequest();

private:
    void trackMceProperty(
            const QString &changedSignal,
            const char *changedSlot,
            const QString &getMethod,
            void (MceDeviceLock::*replySlot)(const QString &));

    void setStateAndSetupLockTimer();
    bool getRequiredLockState();
    bool needLockTimer();

    MceDeviceLockAdaptor m_adaptor;
    NemoDBus::Interface m_mceRequest;

    BackgroundActivity m_hbTimer;

    bool m_locked;
    bool m_callActive;
    bool m_displayOn;
    bool m_tklockActive;
    bool m_userActivity;
    bool m_lpmMode;

    friend class MceDeviceLockAdaptor;

#ifdef UNIT_TEST
    friend class Ut_DeviceLock;
#endif
};

}

#endif
