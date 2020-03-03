/*
 * Copyright 2020 The Android Open Source Project
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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.identity;

/**
 * Cipher suites that can be used for communication between holder and reader devices.
 */
@VintfStability
@Backing(type="int")
enum CipherSuite {
    /**
     * Specifies that the cipher suite that will be used to secure communications between the reader
     * and the prover is:
     *
     *  - For ECDH, ECKA-DH (Elliptic Curve Key Agreement Algorithm - Diffie-Hellman)
     *    according to BSI TR03111 shall be used. The output of this function is the shared secret
     *    value Zab.
     *
     *  - The key derivation shall use HKDF instantiated with SHA-256 as defined in RFC 5869.
     *    The info parameter shall be empty, the output key length is 256 bits. Two keys shall be
     *    derived, SKReader shall be derived using a salt of 0x00, SKDevice shall be derived using
     *    a salt of 0x01.
     *
     *  - For encryption AES-256-GCM (GCM: Galois Counter Mode) shall be used. The reader shall
     *    encrypt its messages with SKReader, the prover shall encrypt its messages with SKDevice.
     *    Therefore, both the prover and the reader need to generate both session keys in order to
     *    be able to also decrypt the messages they receive. The nonce used for encryption shall be
     *    built up according to the following structure: identifier | counter. The identifier is
     *    an 8 byte value. The reader shall use the following identifier: 0x00 0x00 0x00 0x00
     *    0x00 0x00 0x00 0x00. The provier shall use the following identifier: 0x00 0x00 0x00 0x00
     *    0x00 0x00 0x00 0x01. Each session key has its own counter value. The counter value is an
     *    unsigned integer. The first encryption with a key shall use a counter value of 1. For
     *    each following encryption the counter value shall be increased by 1. The counter value
     *    shall be formatted as a 4 byte big endian value. A counter value shall never be reused
     *    in any future encryption using the same key. For the encryption, the IV is the nonce
     *    value and the AAD is an empty string. The format of the encrypted message is the
     *    ciphertext, followed by 16 bytes of the tag.
     *
     * At present this is the only supported cipher suite and it is mandatory for all
     * implementations to support it.
     */
    CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256 = 1,
}
