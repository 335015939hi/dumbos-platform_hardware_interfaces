/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

/**
 * This is the definition of the data format of an Arc.
 * @hide
 */
@VintfStability
parcelable Arc {
    /**
     * The messages exchanged between the domains in the Authgraph protocol are called Arcs.
     * An arc is simply AES-GCM. Encryption of a payload P with a key K and additional
     * authentication data (AAD) D: (i.e. Arc = Enc(K, P, D)). Payload can be a COSE key, another
     * arc or an arbitrary byte array.
     *
     * The CDDL of an arc is as follows.
     * Arc = [                               ; COSE_Encrypt0
     *     1, ; version
     *     protected : bstr .cbor ArcProtectedHeaders,
     *     unprotected : {
     *         5 : bstr .size 12          ; IV
     *     },
     *     ciphertext : bstr    ; Enc(K, bstr .cbor DestinationNode/ bstr .cbor Arc/ bstr,
     *                          ;     encoded ArcEncStruct)
     * ]
     *
     * ArcProtectedHeaders = {
     *            1 : A256GCM		     ; Algorithm: AES-GCM mode w/ 256-bit key, 128-bit tag
     *     ? -70001 : [{ + Permission }] ; One or more Permissions
     *     ? -70002 : [{ + Limitation }] ; One or more Limitations
     *       -70003 : int    ; Timestamp in milliseconds since some starting point (generally
     *                       ; the most recent device boot) which all of the applications within
     *                       ; the secure domain must agree upon
     *     ? -7004  : bstr .size 16      ; Nonce used in channel establishment methods
     * }
     *
     * Permission = &(
     *     -4770552 : Identity,  ; "source_id" - in the source specific operations, the source adds
     *                           ; its own identity to the permissions of an arc.
     *     -4770553 : Identity,  ; "sink_id" - in the sink specific operations, the sink adds its
     *                           ; own identity to the permissions of an arc.
     *     -4770554 : bstr,      ; "challenge" - should be added to the arcs that encrypts the
     *                           ; auth_key. The challenge is returned by the begin() operation of
     *                           ; keymint returns a challenge.
     *     -4770555 : [ +Identity ]         ; "minting_allowed" - dsefines the set of TA identities
     *                                      ; to whom the payload key is allowed to be minted.
     *     -4770556 : null                  ; "deleted_on_biometric_change" - A Boolean value that
     *                                      ; indicates whether an auth key issued from a biometric
     *                                      ; TA is invalidated on new biometric enrollment or
     *                                      ; removal of all biometrics.
     * )
     *
     * As the first step, the identity of a party is defined as their public signing key. In the
     * future, this will be updated to include DICE certificate chain and the identity verification
     * policy.
     *
     * Identity = bstr .cbor &(
     *     PubKeyEd25519,
     *     PubKeyECDSA256,
     * )
     *
     * PubKeyEd25519 = {               ; COSE_Key
     *     1 : 1,                      ; Key type : octet key pair
     *     3 : AlgorithmEdDSA (-8),    ; Algorithm : EdDSA
     *    -1 : 6,                      ; Curve : Ed25519
     *    -2 : bstr                    ; X coordinate, little-endian
     * }
     *
     * PubKeyECDSA256 = {            ; COSE_Key
     *      1 : 2, 			         ; Key type : EC2
     *      3 : AlgorithmES256 (-7), ; Algorithm : ECDSA w/ SHA-256
     *      4 : [ 2 ], 			     ; Key_ops: [verify]
     *     -1 : 1,			         ; Curve: P256
     *     -2 : bstr				 ; X coordinate, big-endian
     *     -3 : bstr				 ; Y coordinate, big-endian
     * }
     *
     * DestinationNode = &(       ; One of the three payload types of an Arc is a secret key
	 *     SymmetricKey,
     *     ECPrivateKey,
     * )
     *
     * ECPrivateKey = &(    ; Private key of an key pair generated for key agreement
     *     PrivateKeyX25519,
     *     PrivateKeyP256,
     * )
     *
     * PrivateKeyX25519 = {
     *      1 : 1,                            ; Key type : Octet Key Pair
     *      3 : ECDH-ES + HKDF-256 (-25),	  ; Algorithm: ECDH ES w/ HKDF - generate key directly
     *      4 : [7],                          ; Key_ops: [derive key]
     *     -1 : 4                             ; Curve: X25519
     *     -4 : bstr                          ; private key
     * }
     *
     * PrivateKeyP256 = {               ; COSE_Key
     *        1 : 2,                    ; Key type : EC2
     *        3 : ECDH-ES + HKDF-256,	; Algorithm: ECDH ES w/ HKDF - generate key directly
     *        4 : [7],                  ; Key_ops: [derive key]
     *       -1 : 1                     ; Curve: P-256
     *     ? -2 : bstr                  ; x coordinate
     *     ? -3 : bstr                  ; y coordinate
     *       -4 : bstr                  ; private key
     * }
     *
     * SymmetricKey = {              ; COSE_Key - For symmetric key encryption
     *     1 : 4,					 ; Key type : Symmetric
     *     3 : A256GCM (3),          ; Algorithm : AES-GCM mode w/ 256-bit key, 128-bit tag
     *     4 : [ 4 ], 				 ; Key_ops: [decrypt]
     *     k : bstr .size 32,	     ; Key value
     * }
     *
     * ArcEncStruct = [              ; COSE_Enc_structure
     *     context   : "Encrypt0",
     *     protected : bstr .cbor ArcProtectedHeaders,
     *     external_aad : bstr .size 0,
     * ]
     *
     */
    byte[] Arc;
}
