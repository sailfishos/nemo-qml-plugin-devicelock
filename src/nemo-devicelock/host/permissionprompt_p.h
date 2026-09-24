/* Copyright (C) 2026 Jolla Mobile Ltd
 * BSD 3-Clause License, see LICENSE.
 */
#ifndef NEMODEVICELOCK_PERMISSIONPROMPT_P_H
#define NEMODEVICELOCK_PERMISSIONPROMPT_P_H

#include <QFileInfo>
#include <QVariantMap>

namespace NemoDeviceLock {

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
        data.insert(QStringLiteral("instruction"), properties.value(QStringLiteral("instruction")).toString());
    }
    return data;
}

}

#endif
