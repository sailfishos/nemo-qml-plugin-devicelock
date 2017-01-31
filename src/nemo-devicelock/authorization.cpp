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

#include "authorization.h"

namespace NemoDeviceLock
{

/*!
    \class NemoDeviceLock::Authorization
    \brief The Authorization class is an interface implemented by types which require user authorization to perform a task.

    An API which requires user authorization will produce a challenge code in response to the
    requestChallenge() function.  This code can be passed to an Authenticator to produce an
    authentication token that can be passed as proof of authentication to other members of the
    API.
*/

/*!
    Constructs an authorization instance which is a child of \a parent.
*/

Authorization::Authorization(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys an authorization instance.
*/

Authorization::~Authorization()
{
}

/*!
    \property NemoDeviceLock::Authorization::allowedMethods

    This property holds the authentication methods that may be used to authenticate the challenge
    code.  An authentication token issued using a different method will be rejected.
*/

/*!
    \property NemoDeviceLock::Authorization::status

    This property holds the status of the authorization challenge.
*/

/*!
    \property NemoDeviceLock::Authorization::challengeCode

    This property holds the issued challenge code.
*/

/*!
    \fn NemoDeviceLock::Authorization::requestChallege()

    Requests that a challenge code be issued.
*/

/*!
    \fn NemoDeviceLock::Authorization::relinquishChallenge()

    Abandons a previously requested challenge.
*/

/*!
    \signal NemoDeviceLock::Authorization::challengeIssued()

    Signals that a requested challenge was successfully issued.
*/

/*!
    \signal NemoDeviceLock::Authorization::challengeDenied()

    Signals that a requested challenge was denied.
*/

/*!
    \signal NemoDeviceLock::Authorization::challengeExpired()

    Signals that a challenge that had beed successfully issued has now expired.
*/

}
