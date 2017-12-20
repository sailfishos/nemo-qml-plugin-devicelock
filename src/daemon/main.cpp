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

#include <cliauthenticator.h>
#include <clidevicelock.h>
#include <clidevicelocksettings.h>
#include <clidevicereset.h>
#include <cliencryptionsettings.h>
#include <clivalidator.h>
#include <hostfingerprintsensor.h>
#include <hostfingerprintsettings.h>
#include <hostservice.h>

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    NemoDeviceLock::CliAuthenticator authenticator;
    NemoDeviceLock::CliDeviceLock deviceLock;
    NemoDeviceLock::CliDeviceLockSettings deviceLockSettings;
    NemoDeviceLock::CliDeviceReset deviceReset;
    NemoDeviceLock::CliEncryptionSettings encryptionSettings;
    NemoDeviceLock::CliValidator validator;
    NemoDeviceLock::HostFingerprintSensor fingerprintSensor;
    NemoDeviceLock::HostFingerprintSettings fingerprintSettings;

    NemoDeviceLock::HostService service(
                &authenticator,
                &deviceLock,
                &deviceLockSettings,
                &deviceReset,
                &encryptionSettings,
                &fingerprintSensor,
                &fingerprintSettings,
                &validator);

    return application.exec();
}
