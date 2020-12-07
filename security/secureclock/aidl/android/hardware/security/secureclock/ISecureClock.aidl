/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * limitations under the License.
 */

package android.hardware.security.secureclock;
import android.hardware.security.secureclock.TimeStampToken;

/**
 * Secure Clock definition.
 *
 * An ISecureClock provides a keymint service to generate secure timestamp using a secure platform.
 * The secure time stamp contains time in milliseconds i.e. keymint Timestamp aidl. This time stamp also
 * contains a 256 bit MAC which provides integrity protection. The MAC is generated using 256 bit
 * HMAC using shared secret key. The shared secret key must be available to secure clock service by
 * implementing ISharedSecret aidl. Note: ISecureClock depends on shared secret, without which the secure
 * time stamp token cannot be generated.
 */

@VintfStability
interface ISecureClock {

/**
 * Generate Time Stamp.
 *
 * Client of this interface, e.g. Keystore, is required to call this method with the challenge
 * returned by StrongBox begin(). Challenge is a nonce that prevents replay of the timestamp.
 *
 * @param the challenge sent by the client.
 *
 * @return token is the time stamp token.  See TimeStampToken for details.
 */
    TimeStampToken generateTimeStamp(in long challenge);
}
