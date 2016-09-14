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

#ifndef NEMODEVICELOCK_LOCKCODEWATCHER_H
#define NEMODEVICELOCK_LOCKCODEWATCHER_H

#include <QDateTime>
#include <QPointer>
#include <QProcess>
#include <QSharedData>
#include <QVector>

namespace NemoDeviceLock
{

class PluginCommand : public QProcess
{
    Q_OBJECT
public:
    PluginCommand(QObject *caller);

    template <typename Success>  void onSuccess(const Success &success)
    {
        connect(this, &PluginCommand::succeeded, success);
    }

    template <typename Failure>  void onFailure(const Failure &failure)
    {
        connect(this, &PluginCommand::failed, failure);
    }

signals:
    void succeeded();
    void failed();

private:
    friend class LockCodeWatcher;

    ~PluginCommand();

    void processFinished(int exitCode, QProcess::ExitStatus status);

    QPointer<QObject> m_caller;
};

class LockCodeWatcher : public QObject, public QSharedData
{
    Q_OBJECT
public:
    ~LockCodeWatcher();

    static LockCodeWatcher *instance();

    bool lockCodeSet() const;
    void invalidateLockCodeSet();

    PluginCommand *checkCode(QObject *caller, const QString &code);
    PluginCommand *unlock(QObject *caller, const QString &code);

    PluginCommand *runPlugin(QObject *caller, const QStringList &arguments) const;

signals:
    void lockCodeSetChanged();

private slots:
    void lockCodeSetInvalidated();

private:
    explicit LockCodeWatcher(QObject *parent = nullptr);

    const bool m_pluginExists;
    mutable bool m_lockCodeSet;
    mutable bool m_codeSetInvalidated;

    static LockCodeWatcher *sharedInstance;
};

}

#endif
