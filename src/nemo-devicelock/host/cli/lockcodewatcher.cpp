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

#include "lockcodewatcher.h"

#include "cliauthenticator.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

namespace NemoDeviceLock
{

PluginCommand::PluginCommand(QObject *caller)
    : m_caller(caller)
{
    connect(static_cast<QProcess *>(this), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &PluginCommand::processFinished);
}

void PluginCommand::processFinished(int exitCode, QProcess::ExitStatus status)
{
    deleteLater();

    QByteArray output = readAllStandardOutput();
    if (!output.isEmpty()) {
        qDebug() << output.constData();
    }

    output = readAllStandardError();
    if (!output.isEmpty()) {
        qWarning() << output.constData();
    }

    if (!m_caller) {
        return;
    } else if (exitCode == 0 && status == QProcess::NormalExit) {
        emit succeeded();
    } else {
        emit failed(exitCode);
    }
}

PluginCommand::~PluginCommand()
{
}

static QString pluginName()
{
    static const QString pluginName = []() {
        QSettings settings(QStringLiteral("/usr/share/lipstick/devicelock/devicelock.conf"), QSettings::IniFormat);
        const QString pluginName = settings.value(QStringLiteral("DeviceLock/pluginName")).toString();

        if (pluginName.isEmpty()) {
            qWarning("DeviceLock: no plugin configuration set in /usr/share/lipstick/devicelock/devicelock.conf");
        }

        return pluginName;
    }();

    return pluginName;
}

LockCodeWatcher *LockCodeWatcher::sharedInstance = nullptr;

LockCodeWatcher::LockCodeWatcher(QObject *parent)
    : QObject(parent)
    , m_pluginExists(QFile::exists(pluginName()))
    , m_lockCodeSet(false)
    , m_codeSetInvalidated(true)
{
    Q_ASSERT(!sharedInstance);
    sharedInstance = this;
}

LockCodeWatcher::~LockCodeWatcher()
{
    sharedInstance = nullptr;
}

LockCodeWatcher *LockCodeWatcher::instance()
{
    return sharedInstance ? sharedInstance : new LockCodeWatcher;
}

bool LockCodeWatcher::lockCodeSet() const
{
    if (m_codeSetInvalidated) {
        m_codeSetInvalidated = false;
        m_lockCodeSet = false;
        if (PluginCommand *command = runPlugin(nullptr, QStringList()
                    << QStringLiteral("--is-set") << QStringLiteral("lockcode"))) {
            command->waitForFinished();
            m_lockCodeSet = command->exitCode() == 0;
        }
    }
    return m_lockCodeSet;
}

void LockCodeWatcher::invalidateLockCodeSet()
{
    if (!m_codeSetInvalidated) {
        m_codeSetInvalidated = true;

        emit lockCodeSetChanged();
    }
}

PluginCommand *LockCodeWatcher::checkCode(QObject *caller, const QString &code)
{
    return runPlugin(caller, QStringList() << QStringLiteral("--check-code") << code);
}

PluginCommand *LockCodeWatcher::unlock(QObject *caller, const QString &code)
{
    return runPlugin(caller, QStringList() << QStringLiteral("--unlock") << code);
}

PluginCommand *LockCodeWatcher::runPlugin(QObject *caller, const QStringList &arguments) const
{
    if (!m_pluginExists) {
        return nullptr;
    }

    PluginCommand *const process = new PluginCommand(caller);

    process->start(pluginName(), arguments);

    return process;
}

void LockCodeWatcher::lockCodeSetInvalidated()
{
    if (!m_codeSetInvalidated) {
        m_codeSetInvalidated = true;
        emit lockCodeSetChanged();
    }
}

}
