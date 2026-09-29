/* Copyright (C) 2026 Jolla Mobile Ltd
 * BSD 3-Clause License, see LICENSE.
 */
#ifndef NEMODEVICELOCK_PERMISSIONPROMPT_P_H
#define NEMODEVICELOCK_PERMISSIONPROMPT_P_H

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QVariantMap>

namespace NemoDeviceLock {

// Resolve display metadata in the privileged host. The UI's credentials do
// not necessarily allow it to inspect the authenticating process's executable.
inline QString permissionPromptExecutable(uint pid)
{
    if (!pid) {
        return QString();
    }
    const QString executable = QFileInfo(QStringLiteral("/proc/%1/exe").arg(pid)).symLinkTarget();
    if (!executable.startsWith(QStringLiteral("/usr/libexec/mapplauncherd/booster-"))) {
        return executable;
    }

    const QFileInfo launcher(executable);
    if (!launcher.isFile() || launcher.ownerId() != 0
            || (launcher.permissions() & (QFile::WriteGroup | QFile::WriteOther))) {
        return executable;
    }
    QFile groups(QStringLiteral("/proc/%1/cgroup").arg(pid));
    QFile maps(QStringLiteral("/proc/%1/maps").arg(pid));
    if (!groups.open(QIODevice::ReadOnly) || !maps.open(QIODevice::ReadOnly)) {
        return executable;
    }
    const QByteArray mappings = maps.readAll();
    for (const QByteArray &line : groups.readAll().split('\n')) {
        const auto fields = line.split(':');
        if (fields.size() != 3 || !fields.at(1).split(',').contains("name=booster")) {
            continue;
        }
        const QString application = QString::fromUtf8(fields.at(2));
        if (!QRegularExpression(QStringLiteral("^/usr/bin/[a-zA-Z0-9_.+-]+$")).match(application).hasMatch()) {
            continue;
        }
        const QFileInfo file(application);
        if (!file.isFile() || file.ownerId() != 0
                || (file.permissions() & (QFile::WriteGroup | QFile::WriteOther))) {
            continue;
        }
        // Booster cgroups can be user-owned. Corroborate the display name
        // with a loaded executable mapping, never argv or a window title.
        // This is not an authorization decision or a publisher attestation.
        const QRegularExpression mapping(QStringLiteral(
                "^[0-9a-f]+-[0-9a-f]+ r-x[p-s] [0-9a-f]+ [0-9a-f]+:[0-9a-f]+ [0-9]+ +%1$")
                .arg(QRegularExpression::escape(file.canonicalFilePath())));
        for (const QByteArray &entry : mappings.split('\n')) {
            if (mapping.match(QString::fromUtf8(entry)).hasMatch()) {
                return application;
            }
        }
    }
    return executable;
}

inline QVariantMap permissionPromptData(uint peerPid, const QString &message,
                                       const QVariantMap &properties)
{
    QVariantMap data {{QStringLiteral("message"), message}};
    if (!peerPid) {
        return data;
    }

    // Only the real Secrets daemon may attest another application's identity.
    // peerPid comes from the authenticated D-Bus connection, never properties.
    const QString executable = QFileInfo(QStringLiteral("/proc/%1/exe").arg(peerPid)).symLinkTarget();
    const QFileInfo file(executable);
    if (executable == QStringLiteral("/usr/bin/sailfishsecretsd")
            && file.ownerId() == 0
            && !(file.permissions() & (QFile::WriteGroup | QFile::WriteOther))
            && properties.value(QStringLiteral("secretsPrompt")).toBool()) {
        data.insert(QStringLiteral("secretsPrompt"), true);
        const uint callerPid = properties.value(QStringLiteral("authenticatingPid"), peerPid).toUInt();
        data.insert(QStringLiteral("applicationExecutable"), permissionPromptExecutable(callerPid));
        data.insert(QStringLiteral("instruction"), properties.value(QStringLiteral("instruction")).toString());
    }
    return data;
}

}

#endif
