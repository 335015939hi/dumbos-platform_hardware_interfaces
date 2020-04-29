/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.hardware.keymaster @5.0;

import android.hardware.keymaster_capability @1.0;
import KeymasterOperation;
import android.hardware.identity.SecuityLevel;

/**
 * Keymaster device definition.
 *
 * == Features ==
 *
 * An IKeymasterDevice provides cryptographic services, including the following categories of
 * operations:
 *
 * o   Key generation
 * o   Import and export (public only) of asymmetric keys
 * o   Import of raw symmetric keys
 * o   Asymmetric decryption with appropriate padding modes
 * o   Asymmetric signing with digesting and appropriate padding modes
 * o   Symmetric encryption and decryption in appropriate modes, including an AEAD mode
 * o   Generation and verification of symmetric message authentication codes
 * o   Attestation to the presence and configuration of asymmetric keys.
 *
 * Protocol elements, such as purpose, mode and padding, as well as access control constraints, must
 * be specified by the caller when keys are generated or imported and must be permanently bound to
 * the key, ensuring that the key cannot be used in any other way.
 *
 * In addition to the list above, IKeymasterDevice implementations must provide one more service
 * which is not exposed as an API but used internally: Random number generation.  The random number
 * generator must be high-quality and must be used for generation of keys, initialization vectors,
 * random padding and other elements of secure protocols that require randomness.
 *
 * == Types of IKeymasterDevices ==
 *
 * All of the operations and storage of key material must occur in a secure environment.  Secure
 * environments may be either:
 *
 * 1.  Isolated execution environments, such as a separate virtual machine, hypervisor or
 *      purpose-built trusted execution environment like ARM TrustZone.  The isolated environment
 *      must provide complete separation from the Android kernel and user space (collectively called
 *      the "non-secure world", or NSW) so that nothing running in the NSW can observe or manipulate
 *      the results of any computation in the isolated environment.  Isolated execution environments
 *      are identified by the SecurityLevel TRUSTED_ENVIRONMENT.
 *
 * 2.  Completely separate, purpose-built and certified secure CPUs, called "StrongBox" devices.
 *      Examples of StrongBox devices are embedded Secure Elements (eSE) or on-SoC secure processing
 *      units (SPU).  StrongBox environments are identified by the SecurityLevel STRONGBOX.  To
 *      qualify as a StrongBox, a device must meet the requirements specified in CDD 9.11.2.
 *
 * == Necessary Primitives ==
 *
 * All IKeymasterDevice implementations must provide support for the following:
 *
 * o   RSA
 *
 *      - TRUSTED_ENVIRONMENT IKeymasterDevices must support 2048, 3072 and 4096-bit keys.
 *        STRONGBOX IKeymasterDevices must support 2048-bit keys.
 *      - Public exponent F4 (2^16+1)
 *      - Unpadded, RSASSA-PSS and RSASSA-PKCS1-v1_5 padding modes for RSA signing
 *      - TRUSTED_ENVIRONMENT IKeymasterDevices must support MD5, SHA1, SHA-2 224, SHA-2 256, SHA-2
 *        384 and SHA-2 512 digest modes for RSA signing.  STRONGBOX IKeymasterDevices must support
 *        SHA-2 256.
 *      - Unpadded, RSAES-OAEP and RSAES-PKCS1-v1_5 padding modes for RSA encryption.
 *
 * o   ECDSA
 *
 *      - TRUSTED_ENVIRONMENT IKeymasterDevices must support NIST curves P-224, P-256, P-384 and
 *        P-521.  STRONGBOX IKeymasterDevices must support NIST curve P-256.
 *      - TRUSTED_ENVIRONMENT IKeymasterDevices must support SHA1, SHA-2 224, SHA-2 256, SHA-2
 *        384 and SHA-2 512 digest modes.  STRONGBOX IKeymasterDevices must support SHA-2 256.
 *
 * o   AES
 *
 *      - 128 and 256-bit keys
 *      - CBC, CTR, ECB and GCM modes.  The GCM mode must not allow the use of tags smaller than 96
 *        bits or nonce lengths other than 96 bits.
 *      - CBC and ECB modes must support unpadded and PKCS7 padding modes.  With no padding CBC and
 *        ECB-mode operations must fail with ErrorCode::INVALID_INPUT_LENGTH if the input isn't a
 *        multiple of the AES block size.  With PKCS7 padding, GCM and CTR operations must fail with
 *        ErrorCode::INCOMPATIBLE_PADDING_MODE.
 *
 * o   3DES
 *
 *      - 168-bit keys.
 *      - CBC and ECB mode.

 *      - CBC and ECB modes must support unpadded and PKCS7 padding modes.  With no padding CBC and
 *        ECB-mode operations must fail with ErrorCode::INVALID_INPUT_LENGTH if the input isn't a
 *        multiple of the DES block size.
 *
 * o   HMAC
 *
 *      - Any key size that is between 64 and 512 bits (inclusive) and a multiple of 8 must be
 *        supported.  STRONGBOX IKeymasterDevices must not support keys larger than 512 bits.
 *      - TRUSTED_ENVIRONMENT IKeymasterDevices must support MD-5, SHA1, SHA-2-224, SHA-2-256,
 *        SHA-2-384 and SHA-2-512.  STRONGBOX IKeymasterDevices must support SHA-2-256.
 *
 * == Key Access Control ==
 *
 * Hardware-based keys that can never be extracted from the device don't provide much security if an
 * attacker can use them at will (though they're more secure than keys which can be
 * exfiltrated).  Therefore, IKeymasterDevice must enforce access controls.
 *
 * Access controls are defined as an "authorization list" of tag/value pairs.  Authorization tags
 * are 32-bit integers from the Tag enum, and the values are a variety of types, defined in the
 * TagType enum.  Some tags may be repeated to specify multiple values.  Whether a tag may be
 * repeated is specified in the documentation for the tag and in the TagType.  When a key is created
 * or imported, the caller specifies an authorization list.  The IKeymasterDevice must divide the
 * caller-provided authorizations into two lists, those it enforces in hardware and those it does
 * not.  These two lists are returned as the "hardwareEnforced" and "softwareEnforced" elements of
 * the KeyCharacteristics struct.  The IKeymasterDevice must also add the following authorizations
 * to the appropriate list:
 *
 * o    Tag::OS_VERSION, must be hardware-enforced.
 * o    Tag::OS_PATCHLEVEL, must be hardware-enforced.
 * o    Tag::VENDOR_PATCHLEVEL, must be hardware-enforced.
 * o    Tag::BOOT_PATCHLEVEL, must be hardware-enforced.
 * o    Tag::CREATION_DATETIME, must be software-enforced, unless the IKeymasterDevice has access to
 *      a secure time service.
 * o    Tag::ORIGIN, must be hardware-enforced.
 *
 * The IKeymasterDevice must accept arbitrary, unknown tags and return them in the softwareEnforced
 * list.
 *
 * All authorization tags and their values, both hardwareEnforced and softwareEnforced, including
 * unknown tags, must be cryptographically bound to the private/secret key material such that any
 * modification of the portion of the key blob that contains the authorization list makes it
 * impossible for the secure environment to obtain the private/secret key material.  The recommended
 * approach to meet this requirement is to use the full set of authorization tags associated with a
 * key as input to a secure key derivation function used to derive a key that is used to encrypt the
 * private/secret key material.
 *
 * IKeymasterDevice implementations must place any tags they cannot fully and completely enforce in
 * the softwareEnforced list.  For example, Tag::ORIGINATION_EXPIRE_DATETIME provides the date and
 * time after which a key may not be used to encrypt or sign new messages.  Unless the
 * IKeymasterDevice has access to a secure source of current date/time information, it is not
 * possible for the IKeymasterDevice to enforce this tag.  An IKeymasterDevice implementation may
 * not rely on the non-secure world's notion of time, because it could be controlled by an attacker.
 * Similarly, it cannot rely on GPSr time, even if it has exclusive control of the GPSr, because
 * that might be spoofed by attacker RF signals.
 *
 * It is recommended that IKeymasterDevices not enforce any tags they place in the softwareEnforced
 * list.  The IKeymasterDevice caller must enforce them, and it is unnecessary to enforce them
 * twice.
 *
 * Some tags must be enforced by the IKeymasterDevice.  See the detailed documentation on each Tag
 * in types.hal.
 *
 * == Root of Trust Binding ==
 *
 * IKeymasterDevice keys must be bound to a root of trust, which is a bitstring that must be
 * provided to the secure environment (by an unspecified, implementation-defined mechanism) during
 * startup, preferably by the bootloader.  This bitstring must be cryptographically bound to every
 * key managed by the IKeymasterDevice.  As above, the recommended mechanism for this cryptographic
 * binding is to include the Root of Trust data in the input to the key derivation function used to
 * derive a key that is used to encryp the private/secret key material.
 *
 * The root of trust consists of a bitstring that must be derived from the public key used by
 * Verified Boot to verify the signature on the boot image and from the lock state of the
 * device.  If the public key is changed to allow a different system image to be used or if the lock
 * state is changed, then all of the IKeymasterDevice-protected keys created by the previous system
 * state must be unusable, unless the previous state is restored.  The goal is to increase the value
 * of the software-enforced key access controls by making it impossible for an attacker-installed
 * operating system to use IKeymasterDevice keys.
 *
 * == Version Binding ==
 *
 * All keys must also be bound to the operating system and patch level of the system image and the
 * patch levels of the vendor image and boot image.  This ensures that an attacker who discovers a
 * weakness in an old version of the software cannot roll a device back to the vulnerable version
 * and use keys created with the newer version.  In addition, when a key with a given version and
 * patch level is used on a device that has been upgraded to a newer version or patch level, the key
 * must be upgraded (See IKeymasterDevice::upgradeKey()) before it can be used, and the previous
 * version of the key must be invalidated.  In this way, as the device is upgraded, the keys will
 * "ratchet" forward along with the device, but any reversion of the device to a previous release
 * will cause the keys to be unusable.
 *
 * This version information must be associated with every key as a set of tag/value pairs in the
 * hardwareEnforced authorization list.  Tag::OS_VERSION, Tag::OS_PATCHLEVEL,
 * Tag::VENDOR_PATCHLEVEL, and Tag::BOOT_PATCHLEVEL must be cryptographically bound to every
 * IKeymasterDevice key, as described in the Key Access Control section above.
 */
@VintfStability interface IKeymasterDevice {
    /**
     * Returns information about the underlying IKeymasterDevice hardware.
     *
     * @return security level of the IKeymasterDevice implementation accessed through this HAL.
     *
     * @return keymasterName is the name of the IKeymasterDevice implementation.
     *
     * @return keymasterAuthorName is the name of the author of the IKeymasterDevice implementation
     *         (organization name, not individual).
     */
    void getHardwareInfo() generates(out SecurityLevel securityLevel, out string keymasterName,
                                     out string keymasterAuthorName);

    /**
     * Verify authorizations for another IKeymasterDevice instance.
     *
     * On systems with both a StrongBox and a TEE IKeymasterDevice instance it is sometimes useful
     * to ask the TEE KeymasterDevice to verify authorizations for a key hosted in StrongBox.
     *
     * For every StrongBox operation, Keystore is required to call this method on the TEE Keymaster,
     * passing in the StrongBox key's hardwareEnforced authorization list and the operation handle
     * returned by StrongBox begin().  The TEE KeymasterDevice must validate all of the
     * authorizations it can and return those it validated in the VerificationToken.  If it cannot
     * verify any, the parametersVerified field of the VerificationToken must be empty.  Keystore
     * must then pass the VerificationToken to the subsequent invocations of StrongBox update() and
     * finish().
     *
     * StrongBox implementations must return ErrorCode::UNIMPLEMENTED.
     *
     * @param operationHandle the operation handle returned by StrongBox Keymaster's begin().
     *
     * @param parametersToVerify Set of authorizations to verify.  The caller may provide an empty
     *        vector if the only required information is the TEE timestamp.
     *
     * @param authToken A HardwareAuthToken if needed to authorize key usage.   ????? delete?
     *
     * @return error ErrorCode::OK on success or ErrorCode::UNIMPLEMENTED if the KeymasterDevice is
     *         a StrongBox.  If the IKeymasterDevice cannot verify one or more elements of
     *         parametersToVerify it must not return an error code, but just omit the unverified
     *         parameter from the VerificationToken.
     *
     * @return token the verification token.  See VerificationToken in types.hal for details.
     */
    VerificationToken verifyAuthorization(in KeymasterOperation operation,
                                          in KeyParameter[] parametersToVerify,
                                          in KeymasterCapability capability);

    /**
     * Adds entropy to the RNG used by Keymaster.  Entropy added through this method must not be the
     * only source of entropy used, and a secure mixing function must be used to mix the entropy
     * provided by this method with internally-generated entropy.  The mixing function must be
     * secure in the sense that if any one of the mixing function inputs is provided with any data
     * the attacker cannot predict (or control), then the output of the seeded CRNG is
     * indistinguishable from random.  Thus, if the entropy from any source is good, the output must
     * be good.
     *
     * @param data Bytes to be mixed into the CRNG seed.  The caller must not provide more than 2
     *        KiB of data per invocation.
     *
     * @return error ErrorCode::OK on success; ErrorCode::INVALID_INPUT_LENGTH if the caller
     *         provides more than 2 KiB of data.
     */
    void addRngEntropy(in byte[] data);

    /**
     * Generates a new cryptographic key, specifying associated parameters, which must be
     * cryptographically bound to the key.  IKeymasterDevice implementations must disallow any use
     * of a key in any way inconsistent with the authorizations specified at generation time.  With
     * respect to parameters that the secure environment cannot enforce, the secure envionment's
     * obligation is limited to ensuring that the unenforceable parameters associated with the key
     * cannot be modified, so that every call to getKeyCharacteristics returns the original
     * values.  In addition, the characteristics returned by generateKey places parameters correctly
     * in the hardware-enforced and software-enforced lists.  See getKeyCharacteristics for more
     * details.
     *
     * In addition to the parameters provided, generateKey must add the following to the returned
     * characteristics.
     *
     * o Tag::ORIGIN with the value KeyOrigin::GENERATED.
     *
     * o Tag::BLOB_USAGE_REQUIREMENTS with the appropriate value (see KeyBlobUsageRequirements in
     *   types.hal).
     *
     * o Tag::CREATION_DATETIME with the appropriate value.  Note that it is expected that this will
     *   generally be added by the HAL, not by the secure environment, and that it will be in the
     *   software-enforced list.  It must be cryptographically bound to the key, like all tags.
     *
     * o Tag::OS_VERSION, Tag::OS_PATCHLEVEL, Tag::VENDOR_PATCHLEVEL and Tag::BOOT_PATCHLEVEL with
     *   appropriate values.
     *
     * The parameters provided to generateKey depend on the type of key being generated.  This
     * section summarizes the necessary and optional tags for each type of key.  Tag::ALGORITHM is
     * always necessary, to specify the type.
     *
     * == RSA Keys ==
     *
     * The following parameters are required to generate an RSA key:
     *
     * o Tag::Key_SIZE specifies the size of the public modulus, in bits.  If omitted, generateKey
     *   must return ErrorCode::UNSUPPORTED_KEY_SIZE.  Required values for TEE IKeymasterDevice
     *   implementations are 1024, 2048, 3072 and 4096.  StrongBox IKeymasterDevice implementations
     *   must support 2048.
     *
     * o Tag::RSA_PUBLIC_EXPONENT specifies the RSA public exponent value.  If omitted, generateKey
     *   must return ErrorCode::INVALID_ARGUMENT.  The values 3 and 65537 must be supported.  It is
     *   recommended to support all prime values up to 2^64.  If provided with a non-prime value,
     *   generateKey must return ErrorCode::INVALID_ARGUMENT.
     *
     * The following parameters are not necessary to generate a usable RSA key, but generateKey must
     * not return an error if they are omitted:
     *
     * o Tag::PURPOSE specifies allowed purposes.  All KeyPurpose values (see types.hal) must be
     *   supported for RSA keys.
     *
     * o Tag::DIGEST specifies digest algorithms that may be used with the new key.  TEE
     *   IKeymasterDevice implementatiosn must support all Digest values (see types.hal) for RSA
     *   keys.  StrongBox IKeymasterDevice implementations must support SHA_2_256.
     *
     * o Tag::PADDING specifies the padding modes that may be used with the new
     *   key.  IKeymasterDevice implementations must support PaddingMode::NONE,
     *   PaddingMode::RSA_OAEP, PaddingMode::RSA_PSS, PaddingMode::RSA_PKCS1_1_5_ENCRYPT and
     *   PaddingMode::RSA_PKCS1_1_5_SIGN for RSA keys.
     *
     * == ECDSA Keys ==
     *
     * Either Tag::KEY_SIZE or Tag::EC_CURVE must be provided to generate an ECDSA key.  If neither
     * is provided, generateKey must return ErrorCode::UNSUPPORTED_KEY_SIZE.  If Tag::KEY_SIZE is
     * provided, the possible values are 224, 256, 384 and 521, and must be mapped to Tag::EC_CURVE
     * values P_224, P_256, P_384 and P_521, respectively.  TEE IKeymasterDevice implementations
     * must support all curves.  StrongBox implementations must support P_256.
     *
     * == AES Keys ==
     *
     * Only Tag::KEY_SIZE is required to generate an AES key.  If omitted, generateKey must return
     * ErrorCode::UNSUPPORTED_KEY_SIZE.  128 and 256-bit key sizes must be supported.
     *
     * If Tag::BLOCK_MODE is specified with value BlockMode::GCM, then the caller must also provide
     * Tag::MIN_MAC_LENGTH.  If omitted, generateKey must return ErrorCode::MISSING_MIN_MAC_LENGTH.
     *
     *
     * @param keyParams Key generation parameters are defined as KeymasterDevice tag/value pairs,
     *        provided in params.  See above for detailed specifications of which tags are required
     *        for which types of keys.
     *
     * @return keyBlob Opaque descriptor of the generated key.  The recommended implementation
     *         strategy is to include an encrypted copy of the key material, wrapped in a key
     *         unavailable outside secure hardware.
     *
     * @return keyCharacteristics Description of the generated key.  See the getKeyCharacteristics
     *         method below.
     */
    void generateKey(in KeyParameter[] keyParams, out byte[] generatedKeyBlob,
                     out KeyCharacteristics generatedKeyCharacteristics);

    /**
     * Imports key material into an IKeymasterDevice.  Key definition parameters and return values
     * are the same as for generateKey, with the following exceptions:
     *
     * o Tag::KEY_SIZE is not necessary in the input parameters.  If not provided, the
     *   IKeymasterDevice must deduce the value from the provided key material and add the tag and
     *   value to the key characteristics.  If Tag::KEY_SIZE is provided, the IKeymasterDevice must
     *   validate it against the key material.  In the event of a mismatch, importKey must return
     *   ErrorCode::IMPORT_PARAMETER_MISMATCH.
     *
     * o Tag::RSA_PUBLIC_EXPONENT (for RSA keys only) is not necessary in the input parameters.  If
     *   not provided, the IKeymasterDevice must deduce the value from the provided key material and
     *   add the tag and value to the key characteristics.  If Tag::RSA_PUBLIC_EXPONENT is provided,
     *   the IKeymasterDevice must validate it against the key material.  In the event of a
     *   mismatch, importKey must return ErrorCode::IMPORT_PARAMETER_MISMATCH.
     *
     * o Tag::ORIGIN (returned in keyCharacteristics) must have the value KeyOrigin::IMPORTED.
     *
     * @param keyParams Key generation parameters are defined as KeymasterDevice tag/value pairs,
     *        provided in params.
     *
     * @param keyFormat The format of the key material to import.  See KeyFormat in types.hal.
     *
     * @pram keyData The key material to import, in the format specifed in keyFormat.
     *
     * @return keyBlob Opaque descriptor of the imported key.  The recommended implementation
     *         strategy is to include an encrypted copy of the key material, wrapped in a key
     *         unavailable outside secure hardware.
     *
     * @return keyCharacteristics Decription of the generated key.  See the getKeyCharacteristics
     *         method below.
     */
    void importKey(in KeyParameter[] inKeyParams, in KeyFormat inKeyFormat,
                   in byte[] inKeyData, out byte[] importedKeyBlob,
                   out KeyCharacteristics importedKeyCharacteristics);

    /**
     * Securely imports a key, or key pair, returning a key blob and a description of the imported
     * key.
     *
     * @param wrappedKeyData The wrapped key material to import.  The wrapped key is in DER-encoded
     * ASN.1 format, specified by the following schema:
     *
     *     KeyDescription ::= SEQUENCE(
     *         keyFormat INTEGER,                   # Values from KeyFormat enum.
     *         keyParams AuthorizationList,
     *     )
     *
     *     SecureKeyWrapper ::= SEQUENCE(
     *         version INTEGER,                     # Contains value 0
     *         encryptedTransportKey OCTET_STRING,
     *         initializationVector OCTET_STRING,
     *         keyDescription KeyDescription,
     *         encryptedKey OCTET_STRING,
     *         tag OCTET_STRING
     *     )
     *
     *     Where:
     *
     *     o keyFormat is an integer from the KeyFormat enum, defining the format of the plaintext
     *       key material.
     *     o keyParams is the characteristics of the key to be imported (as with generateKey or
     *       importKey).  If the secure import is successful, these characteristics must be
     *       associated with the key exactly as if the key material had been insecurely imported
     *       with the @3.0::IKeymasterDevice::importKey.  See attestKey() for documentation of the
     *       AuthorizationList schema.
     *     o encryptedTransportKey is a 256-bit AES key, XORed with a masking key and then encrypted
     *       with the wrapping key specified by wrappingKeyBlob.
     *     o keyDescription is a KeyDescription, above.
     *     o encryptedKey is the key material of the key to be imported, in format keyFormat, and
     *       encrypted with encryptedEphemeralKey in AES-GCM mode, with the DER-encoded
     *       representation of keyDescription provided as additional authenticated data.
     *     o tag is the tag produced by the AES-GCM encryption of encryptedKey.
     *
     * So, importWrappedKey does the following:
     *
     *     1. Get the private key material for wrappingKeyBlob, verifying that the wrapping key has
     *        purpose KEY_WRAP, padding mode RSA_OAEP, and digest SHA_2_256, returning the
     *        appropriate error if any of those requirements fail.
     *     2. Extract the encryptedTransportKey field from the SecureKeyWrapper, and decrypt
     *        it with the wrapping key.
     *     3. XOR the result of step 2 with maskingKey.
     *     4. Use the result of step 3 as an AES-GCM key to decrypt encryptedKey, using the encoded
     *        value of keyDescription as the additional authenticated data.  Call the result
     *        "keyData" for the next step.
     *     5. Perform the equivalent of calling importKey(keyParams, keyFormat, keyData), except
     *        that the origin tag should be set to SECURELY_IMPORTED.
     *
     * @param wrappingKeyBlob The opaque key descriptor returned by generateKey() or importKey().
     *        This key must have been created with Purpose::WRAP_KEY.
     *
     * @param maskingKey The 32-byte value XOR'd with the transport key in the SecureWrappedKey
     *        structure.
     *
     * @param unwrappingParams must contain any parameters needed to perform the unwrapping
     *        operation.  For example, if the wrapping key is an AES key the block and padding modes
     *        must be specified in this argument.
     *
     * @param passwordSid specifies the password secure ID (SID) of the user that owns the key being
     *        installed.  If the authorization list in wrappedKeyData contains a Tag::USER_SECURE_ID
     *        with a value that has the HardwareAuthenticatorType::PASSWORD bit set, the constructed
     *        key must be bound to the SID value provided by this argument.  If the wrappedKeyData
     *        does not contain such a tag and value, this argument must be ignored.
     *
     * @param biometricSid specifies the biometric secure ID (SID) of the user that owns the key
     *        being installed.  If the authorization list in wrappedKeyData contains a
     *        Tag::USER_SECURE_ID with a value that has the HardwareAuthenticatorType::FINGERPRINT
     *        bit set, the constructed key must be bound to the SID value provided by this argument.
     *        If the wrappedKeyData does not contain such a tag and value, this argument must be
     *        ignored.
     *
     * @return keyBlob Opaque descriptor of the imported key.  It is recommended that the keyBlob
     *         contain a copy of the key material, wrapped in a key unavailable outside secure
     *         hardware.
     *
     * @return keyCharacteristics Decription of the generated key.  See the getKeyCharacteristics
     *         method below.
     */
    void importWrappedKey(in byte[] inWrappedKeyData, in byte[] inWrappingKeyBlob,
                          in byte[] inMaskingKey,
                          in KeyParameter[] inUnwrappingParams, in uint64_t inPasswordSid,
                          in uint64_t inBiometricSid, out byte[] importedKeyBlob,
                          out KeyCharacteristics importedKeyCharacteristics);

    /**
     * Returns parameters associated with the provided key, divided into two sets: hardware-enforced
     * and software-enforced.  The description here applies equally to the key characteristics lists
     * returned by generateKey, importKey and importWrappedKey.  The characteristics returned by
     * this method completely describe the type and usage of the specified key.
     *
     * The rule that IKeymasterDevice implementations must use for deciding whether a given tag
     * belongs in the hardware-enforced or software-enforced list is that if the meaning of the tag
     * is fully assured by secure hardware, it is hardware enforced.  Otherwise, it's software
     * enforced.
     *
     *
     * @param keyBlob The opaque descriptor returned by generateKey, importKey or importWrappedKey.
     *
     * @param clientId An opaque byte string identifying the client.  This value must match the
     *        Tag::APPLICATION_ID data provided during key generation/import.  Without the correct
     *        value, it must be computationally infeasible for the secure hardware to obtain the key
     *        material.
     *
     * @param appData An opaque byte string provided by the application.  This value must match the
     *        Tag::APPLICATION_DATA data provided during key generation/import.  Without the correct
     *        value, it must be computationally infeasible for the secure hardware to obtain the key
     *        material.
     *
     * @return keyCharacteristics Decription of the generated key.  See KeyCharacteristics in
     *         types.hal.
     */
    KeyCharacteristics getKeyCharacteristics(
            in byte[] keyBlob, in byte[] clientId, in byte[] appData);

    /**
     * Exports a public key, returning the key in the specified format.
     *
     * @parm keyFormat The format used for export.  See KeyFormat in types.hal.
     *
     * @param keyBlob The opaque descriptor returned by generateKey() or importKey().  The
     *        referenced key must be asymmetric.
     *
     * @param clientId An opaque byte string identifying the client.  This value must match the
     *        Tag::APPLICATION_ID data provided during key generation/import.  Without the correct
     *        value, it must be computationally infeasible for the secure hardware to obtain the key
     *        material.
     *
     * @param appData An opaque byte string provided by the application.  This value must match the
     *        Tag::APPLICATION_DATA data provided during key generation/import.  Without the correct
     *        value, it must be computationally infeasible for the secure hardware to obtain the key
     *        material.
     *
     * @return keyMaterial The public key material in PKCS#8 format.
     */
    byte[] exportKey(in KeyFormat keyFormat, in byte[] keyBlob,
                              in byte[] clientId, in byte[] appData);

    /**
     * Generates a signed X.509 certificate chain attesting to the presence of keyToAttest in
     * Keymaster.
     *
     * The certificates in the chain must be ordered such that each certificate is signed by the
     * subsequent one, up to the root which must be self-signed.  The first certificate in the chain
     * signs the public key info of the attested key and must contain the following entries (see RFC
     * 5280 for details on each):
     *
     * o version -- with value 2
     *
     * o serialNumber -- with value 1 (same value for all keys)
     *
     * o signature -- contains an the AlgorithmIdentifier of the algorithm used to sign, must be
     *   ECDSA for EC keys, RSA for RSA keys.
     *
     * o issuer -- must contain the same value as the Subject field of the next certificate.
     *
     * o validity -- SEQUENCE of two dates, containing the values of Tag::ACTIVE_DATETIME and
     *   Tag::USAGE_EXPIRE_DATETIME.  The tag values are in milliseconds since Jan 1, 1970; see RFD
     *   5280 for the correct representation in certificates.  If Tag::ACTIVE_DATETIME is not
     *   present in the key, the IKeymasterDevice must use the value of Tag::CREATION_DATETIME.  If
     *   Tag::USAGE_EXPIRE_DATETIME is not present, the IKeymasterDevice must use the expiration
     *   date of the batch attestation certificate (see below).
     *
     * o subject -- CN="Android Keystore Key" (same value for all keys)
     *
     * o subjectPublicKeyInfo -- X.509 SubjectPublicKeyInfo containing the attested public key.
     *
     * o Key Usage extension -- digitalSignature bit must be set iff the attested key has
     *   KeyPurpose::SIGN.  dataEncipherment bit must be set iff the attested key has
     *   KeyPurpose::DECRYPT.  keyEncipherment bit must be set iff the attested key has
     *   KeyPurpose::KEY_WRAP.  All other bits must be clear.
     *
     * In addition to the above, the attestation certificate must contain an extension with OID
     * 1.3.6.1.4.1.11129.2.1.17 and value according to the KeyDescription schema defined as:
     *
     * KeyDescription ::= SEQUENCE {
     *     attestationVersion         INTEGER, # Value 3
     *     attestationSecurityLevel   SecurityLevel, # See below
     *     keymasterVersion           INTEGER, # Value 4
     *     keymasterSecurityLevel     SecurityLevel, # See below
     *     attestationChallenge       OCTET_STRING, # Tag::ATTESTATION_CHALLENGE from attestParams
     *     uniqueId                   OCTET_STRING, # Empty unless key has Tag::INCLUDE_UNIQUE_ID
     *     softwareEnforced           AuthorizationList, # See below
     *     hardwareEnforced           AuthorizationList, # See below
     * }
     *
     * SecurityLevel ::= ENUMERATED {
     *     Software                   (0),
     *     TrustedEnvironment         (1),
     *     StrongBox                  (2),
     * }
     *
     * RootOfTrust ::= SEQUENCE {
     *     verifiedBootKey            OCTET_STRING,
     *     deviceLocked               BOOLEAN,
     *     verifiedBootState          VerifiedBootState,
     *     # verifiedBootHash must contain 32-byte value that represents the state of all binaries
     *     # or other components validated by verified boot.  Updating any verified binary or
     *     # component must cause this value to change.
     *     verifiedBootHash           OCTET_STRING,
     * }
     *
     * VerifiedBootState ::= ENUMERATED {
     *     Verified                   (0),
     *     SelfSigned                 (1),
     *     Unverified                 (2),
     *     Failed                     (3),
     * }
     *
     * AuthorizationList ::= SEQUENCE {
     *     purpose                    [1] EXPLICIT SET OF INTEGER OPTIONAL,
     *     algorithm                  [2] EXPLICIT INTEGER OPTIONAL,
     *     keySize                    [3] EXPLICIT INTEGER OPTIONAL,
     *     blockMode                  [4] EXPLICIT SET OF INTEGER OPTIONAL,
     *     digest                     [5] EXPLICIT SET OF INTEGER OPTIONAL,
     *     padding                    [6] EXPLICIT SET OF INTEGER OPTIONAL,
     *     callerNonce                [7] EXPLICIT NULL OPTIONAL,
     *     minMacLength               [8] EXPLICIT INTEGER OPTIONAL,
     *     ecCurve                    [10] EXPLICIT INTEGER OPTIONAL,
     *     rsaPublicExponent          [200] EXPLICIT INTEGER OPTIONAL,
     *     rollbackResistance         [303] EXPLICIT NULL OPTIONAL,
     *     activeDateTime             [400] EXPLICIT INTEGER OPTIONAL,
     *     originationExpireDateTime  [401] EXPLICIT INTEGER OPTIONAL,
     *     usageExpireDateTime        [402] EXPLICIT INTEGER OPTIONAL,
     *     userSecureId               [502] EXPLICIT INTEGER OPTIONAL,
     *     noAuthRequired             [503] EXPLICIT NULL OPTIONAL,
     *     userAuthType               [504] EXPLICIT INTEGER OPTIONAL,
     *     authTimeout                [505] EXPLICIT INTEGER OPTIONAL,
     *     allowWhileOnBody           [506] EXPLICIT NULL OPTIONAL,
     *     trustedUserPresenceReq     [507] EXPLICIT NULL OPTIONAL,
     *     trustedConfirmationReq     [508] EXPLICIT NULL OPTIONAL,
     *     unlockedDeviceReq          [509] EXPLICIT NULL OPTIONAL,
     *     creationDateTime           [701] EXPLICIT INTEGER OPTIONAL,
     *     origin                     [702] EXPLICIT INTEGER OPTIONAL,
     *     rootOfTrust                [704] EXPLICIT RootOfTrust OPTIONAL,
     *     osVersion                  [705] EXPLICIT INTEGER OPTIONAL,
     *     osPatchLevel               [706] EXPLICIT INTEGER OPTIONAL,
     *     attestationApplicationId   [709] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdBrand         [710] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdDevice        [711] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdProduct       [712] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdSerial        [713] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdImei          [714] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdMeid          [715] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdManufacturer  [716] EXPLICIT OCTET_STRING OPTIONAL,
     *     attestationIdModel         [717] EXPLICIT OCTET_STRING OPTIONAL,
     *     vendorPatchLevel           [718] EXPLICIT INTEGER OPTIONAL,
     *     bootPatchLevel             [718] EXPLICIT INTEGER OPTIONAL,
     * }
     *
     * The above schema is mostly a straightforward translation of the IKeymasterDevice tag/value
     * parameter lists to ASN.1:
     *
     * o TagType::ENUM, TagType::UINT, TagType::ULONG and TagType::DATE tags are represented as
     *   ASN.1 INTEGER.
     *
     * o TagType::ENUM_REP, TagType::UINT_REP and TagType::ULONG_REP tags are represented as ASN.1
     *   SET of INTEGER.
     *
     * o TagType::BOOL tags are represented as ASN.1 NULL.  All entries in AuthorizationList are
     *   OPTIONAL, so the presence of the tag means "true", absence means "false".
     *
     * o TagType::BYTES tags are represented as ASN.1 OCTET_STRING.
     *
     * The numeric ASN.1 tag numbers are the same values as the IKeymasterDevice Tag enum values,
     * except with the TagType modifier stripped.
     *
     * The attestation certificate must be signed by a "batch" key, which must be securely
     * pre-installed into the device, generally in the factory, and securely stored to prevent
     * access or extraction.  The batch key must be used only for signing attestation certificates.
     * The batch attestation certificate must be signed by a chain or zero or more intermediates
     * leading to a self-signed roots.  The intermediate and root certificate signing keys must not
     * exist anywhere on the device.
     *
     * == ID Attestation ==
     *
     * ID attestation is a special case of key attestation in which unique device ID values are
     * included in the signed attestation certificate.
     *
     * @param keyToAttest The opaque descriptor returned by generateKey() or importKey().  The
     *        referenced key must be asymmetric.
     *
     * @param attestParams Parameters for the attestation.  Must contain Tag::ATTESTATION_CHALLENGE,
     *        the value of which must be put in the attestationChallenge field of the KeyDescription
     *        ASN.1 structure defined above.
     *
     * @return certChain The attestation certificate, and additional certificates back to the root
     *         attestation certificate, which clients will need to check against a known-good value.
     *         The certificates must be DER-encoded.
     */
    byte[][] attestKey(in byte[] keyToAttest,
                                      in KeyParameter[] attestParams);

    /**
     * Upgrades an old key blob.  Keys can become "old" in two ways: IKeymasterDevice can be
     * upgraded to a new version with an incompatible key blob format, or the system can be updated
     * to invalidate the OS version (OS_VERSION tag), system patch level (OS_PATCHLEVEL tag), vendor
     * patch level (VENDOR_PATCH_LEVEL tag), boot patch level (BOOT_PATCH_LEVEL tag) or other,
     * implementation-defined patch level (keymaster implementers are encouraged to extend this HAL
     * with a minor version extension to define validatable patch levels for other images; tags must
     * be defined in the implementer's namespace, starting at 10000).  In either case, attempts to
     * use an old key blob with getKeyCharacteristics(), exportKey(), attestKey() or begin() must
     * result in IKeymasterDevice returning ErrorCode::KEY_REQUIRES_UPGRADE.  The caller must use
     * this method to upgrade the key blob.
     *
     * The upgradeKey method must examine each version or patch level associated with the key.  If
     * any one of them is higher than the corresponding current device value upgradeKey() must
     * return ErrorCode::INVALID_ARGUMENT.  There is one exception: it is always permissible to
     * "downgrade" from any OS_VERSION number to OS_VERSION 0.  For example, if the key has
     * OS_VERSION 080001, it is permisible to upgrade the key if the current system version is
     * 080100, because the new version is larger, or if the current system version is 0, because
     * upgrades to 0 are always allowed.  If the system version were 080000, however, keymaster must
     * return ErrorCode::INVALID_ARGUMENT because that value is smaller than 080001.  Values other
     * than OS_VERSION must never be downgraded.
     *
     * Note that Keymaster versions 2 and 3 required that the system and boot images have the same
     * patch level and OS version.  This requirement is relaxed for 4.0::IKeymasterDevice, and the
     * OS version in the boot image footer is no longer used.
     *
     * @param keyBlobToUpgrade The opaque descriptor returned by generateKey() or importKey();
     *
     * @param upgradeParams A parameter list containing any parameters needed to complete the
     *        upgrade, including Tag::APPLICATION_ID and Tag::APPLICATION_DATA.
     *
     * @return upgradedKeyBlob A new key blob that references the same key as keyBlobToUpgrade, but
     *         is in the new format, or has the new version data.
     */
    byte[] upgradeKey(in byte[] keyBlobToUpgrade,
                               in KeyParameter[] upgradeParams);
    `

            /**
             * Deletes the key, or key pair, associated with the key blob.  Calling this function on
             * a key with Tag::ROLLBACK_RESISTANCE in its hardware-enforced authorization list must
             * render the key permanently unusable.  Keys without Tag::ROLLBACK_RESISTANCE may or
             * may not be rendered unusable.
             *
             * @param keyBlob The opaque descriptor returned by generateKey() or importKey();
             */
            void
            deleteKey(in byte[] keyBlob);

    /**
     * Deletes all keys in the hardware keystore.  Used when keystore is reset completely.  After
     * this function is called all keys with Tag::ROLLBACK_RESISTANCE in their hardware-enforced
     * authorization lists must be rendered permanently unusable.  Keys without
     * Tag::ROLLBACK_RESISTANCE may or may not be rendered unusable.
     *
     * @return error See the ErrorCode enum.
     */
    void deleteAllKeys();

    /**
     * Destroys knowledge of the device's ids.  This prevents all device id attestation in the
     * future.  The destruction must be permanent so that not even a factory reset will restore the
     * device ids.
     *
     * Device id attestation may be provided only if this method is fully implemented, allowing the
     * user to permanently disable device id attestation.  If this cannot be guaranteed, the device
     * must never attest any device ids.
     *
     * This is a NOP if device id attestation is not supported.
     */
    void destroyAttestationIds();

    /**
     * Begins a cryptographic operation using the specified key.  If all is well, begin() must
     * return ErrorCode::OK and create an operation handle which must be passed to subsequent calls
     * to update(), finish() or abort().
     *
     * It is critical that each call to begin() be paired with a subsequent call to finish() or
     * abort(), to allow the IKeymasterDevice implementation to clean up any internal operation
     * state.  The caller's failure to do this may leak internal state space or other internal
     * resources and may eventually cause begin() to return ErrorCode::TOO_MANY_OPERATIONS when it
     * runs out of space for operations.  Any result other than ErrorCode::OK from begin(), update()
     * or finish() implicitly aborts the operation, in which case abort() need not be called (and
     * must return ErrorCode::INVALID_OPERATION_HANDLE if called).  IKeymasterDevice implementations
     * must support 16 concurrent operations.
     *
     * If Tag::APPLICATION_ID or Tag::APPLICATION_DATA were specified during key generation or
     * import, calls to begin must include those tags with the originally-specified values in the
     * inParams argument to this method.  If not, begin() must return ErrorCode::INVALID_KEY_BLOB.
     *
     * == Authorization Enforcement ==
     *
     * The following key authorization parameters must be enforced by the IKeymasterDevice secure
     * environment if the tags were returned in the "hardwareEnforced" list in the
     * KeyCharacteristics.  Public key operations, meaning KeyPurpose::ENCRYPT and
     * KeyPurpose::VERIFY must be allowed to succeed even if authorization requirements are not met.
     *
     * -- All Key Types --
     *
     * The tags in this section apply to all key types.  See below for additional key type-specific
     * tags.
     *
     * o Tag::PURPOSE: The purpose specified in the begin() call must match one of the purposes in
     *   the key authorizations.  If the specified purpose does not match, begin() must return
     *   ErrorCode::UNSUPPORTED_PURPOSE.
     *
     * o Tag::ACTIVE_DATETIME can only be enforced if a trusted UTC time source is available.  If
     *   the current date and time is prior to the tag value, begin() must return
     *   ErrorCode::KEY_NOT_YET_VALID.
     *
     * o Tag::ORIGINATION_EXPIRE_DATETIME can only be enforced if a trusted UTC time source is
     *   available.  If the current date and time is later than the tag value and the purpose is
     *   KeyPurpose::ENCRYPT or KeyPurpose::SIGN, begin() must return ErrorCode::KEY_EXPIRED.
     *
     * o Tag::USAGE_EXPIRE_DATETIME can only be enforced if a trusted UTC time source is
     *   available.  If the current date and time is later than the tag value and the purpose is
     *   KeyPurpose::DECRYPT or KeyPurpose::VERIFY, begin() must return ErrorCode::KEY_EXPIRED.

     * o Tag::MIN_SECONDS_BETWEEN_OPS must be compared with a trusted relative timer indicating the
     *   last use of the key.  If the last use time plus the tag value is less than the current
     *   time, begin() must return ErrorCode::KEY_RATE_LIMIT_EXCEEDED.  See the tag description for
     *   important implementation details.

     * o Tag::MAX_USES_PER_BOOT must be compared against a secure counter that tracks the uses of
     *   the key since boot time.  If the count of previous uses exceeds the tag value, begin() must
     *   return ErrorCode::KEY_MAX_OPS_EXCEEDED.
     *
     * o Tag::USER_SECURE_ID must be enforced by this method if and only if the key also has
     *   Tag::AUTH_TIMEOUT (if it does not have Tag::AUTH_TIMEOUT, the Tag::USER_SECURE_ID
     *   requirement must be enforced by update() and finish()).  If the key has both, then this
     *   method must receive a non-empty HardwareAuthToken in the authToken argument.  For the auth
     *   token to be valid, all of the following have to be true:
     *
     *   o The HMAC field must validate correctly.
     *
     *   o At least one of the Tag::USER_SECURE_ID values from the key must match at least one of
     *     the secure ID values in the token.
     *
     *   o The key must have a Tag::USER_AUTH_TYPE that matches the auth type in the token.
     *
     *   o The timestamp in the auth token plus the value of the Tag::AUTH_TIMEOUT must be less than
     *     the current secure timestamp (which is a monotonic timer counting milliseconds since
     *     boot.)
     *
     *   If any of these conditions are not met, begin() must return
     *   ErrorCode::KEY_USER_NOT_AUTHENTICATED.
     *
     * o Tag::CALLER_NONCE allows the caller to specify a nonce or initialization vector (IV).  If
     *   the key doesn't have this tag, but the caller provided Tag::NONCE to this method,
     *   ErrorCode::CALLER_NONCE_PROHIBITED must be returned.
     *
     * o Tag::BOOTLOADER_ONLY specifies that only the bootloader may use the key.  If this method is
     *   called with a bootloader-only key after the bootloader has finished executing, it must
     *   return ErrorCode::INVALID_KEY_BLOB.  The mechanism for notifying the IKeymasterDevice that
     *   the bootloader has finished executing is implementation-defined.
     *
     * -- RSA Keys --
     *
     * All RSA key operations must specify exactly one padding mode in inParams.  If unspecified or
     * specified more than once, the begin() must return ErrorCode::UNSUPPORTED_PADDING_MODE.
     *
     * RSA signing and verification operations need a digest, as do RSA encryption and decryption
     * operations with OAEP padding mode.  For those cases, the caller must specify exactly one
     * digest in inParams.  If unspecified or specified more than once, begin() must return
     * ErrorCode::UNSUPPORTED_DIGEST.
     *
     * Private key operations (KeyPurpose::DECRYPT and KeyPurpose::SIGN) need authorization of
     * digest and padding, which means that the key authorizations need to contain the specified
     * values.  If not, begin() must return ErrorCode::INCOMPATIBLE_DIGEST or
     * ErrorCode::INCOMPATIBLE_PADDING, as appropriate.  Public key operations (KeyPurpose::ENCRYPT
     * and KeyPurpose::VERIFY) are permitted with unauthorized digest or padding modes.
     *
     * With the exception of PaddingMode::NONE, all RSA padding modes are applicable only to certain
     * purposes.  Specifically, PaddingMode::RSA_PKCS1_1_5_SIGN and PaddingMode::RSA_PSS only
     * support signing and verification, while PaddingMode::RSA_PKCS1_1_5_ENCRYPT and
     * PaddingMode::RSA_OAEP only support encryption and decryption.  begin() must return
     * ErrorCode::UNSUPPORTED_PADDING_MODE if the specified mode does not support the specified
     * purpose.
     *
     * There are some important interactions between padding modes and digests:
     *
     * o PaddingMode::NONE indicates that a "raw" RSA operation is performed.  If signing or
     *   verifying, Digest::NONE is specified for the digest.  No digest is necessary for unpadded
     *   encryption or decryption.
     *
     * o PaddingMode::RSA_PKCS1_1_5_SIGN padding requires a digest.  The digest may be Digest::NONE,
     *   in which case the Keymaster implementation cannot build a proper PKCS#1 v1.5 signature
     *   structure, because it cannot add the DigestInfo structure.  Instead, the IKeymasterDevice
     *   must construct 0x00 || 0x01 || PS || 0x00 || M, where M is the provided message and PS is a
     *   random padding string at least eight bytes in length.  The size of the RSA key has to be at
     *   least 11 bytes larger than the message, otherwise begin() must return
     *   ErrorCode::INVALID_INPUT_LENGTH.
     *
     * o PaddingMode::RSA_PKCS1_1_1_5_ENCRYPT padding does not require a digest.
     *
     * o PaddingMode::RSA_PSS padding requires a digest, which may not be Digest::NONE.  If
     *   Digest::NONE is specified, the begin() must return ErrorCode::INCOMPATIBLE_DIGEST.  In
     *   addition, the size of the RSA key must be at least 2 + D bytes larger than the output size
     *   of the digest, where D is the size of the digest, in bytes.  Otherwise begin() must
     *   return ErrorCode::INCOMPATIBLE_DIGEST.  The salt size must be D.
     *
     * o PaddingMode::RSA_OAEP padding requires a digest, which may not be Digest::NONE.  If
     *   Digest::NONE is specified, begin() must return ErrorCode::INCOMPATIBLE_DIGEST.  The OAEP
     *   mask generation function must be MGF1 and the MGF1 digest must be SHA1, regardless of the
     *   OAEP digest specified.
     *
     * -- EC Keys --
     *
     * EC key operations must specify exactly one padding mode in inParams.  If unspecified or
     * specified more than once, begin() must return ErrorCode::UNSUPPORTED_PADDING_MODE.
     *
     * Private key operations (KeyPurpose::SIGN) need authorization of digest and padding, which
     * means that the key authorizations must contain the specified values.  If not, begin() must
     * return ErrorCode::INCOMPATIBLE_DIGEST.  Public key operations (KeyPurpose::VERIFY) are
     * permitted with unauthorized digest or padding.
     *
     * -- AES Keys --
     *
     * AES key operations must specify exactly one block mode (Tag::BLOCK_MODE) and one padding mode
     * (Tag::PADDING) in inParams.  If either value is unspecified or specified more than once,
     * begin() must return ErrorCode::UNSUPPORTED_BLOCK_MODE or
     * ErrorCode::UNSUPPORTED_PADDING_MODE.  The specified modes must be authorized by the key,
     * otherwise begin() must return ErrorCode::INCOMPATIBLE_BLOCK_MODE or
     * ErrorCode::INCOMPATIBLE_PADDING_MODE.
     *
     * If the block mode is BlockMode::GCM, inParams must specify Tag::MAC_LENGTH, and the specified
     * value must be a multiple of 8 that is not greater than 128 or less than the value of
     * Tag::MIN_MAC_LENGTH in the key authorizations.  For MAC lengths greater than 128 or
     * non-multiples of 8, begin() must return ErrorCode::UNSUPPORTED_MAC_LENGTH.  For values less
     * than the key's minimum length, begin() must return ErrorCode::INVALID_MAC_LENGTH.
     *
     * If the block mode is BlockMode::GCM or BlockMode::CTR, the specified padding mode must be
     * PaddingMode::NONE.  For BlockMode::ECB or BlockMode::CBC, the mode may be PaddingMode::NONE
     * or PaddingMode::PKCS7.  If the padding mode doesn't meet these conditions, begin() must
     * return ErrorCode::INCOMPATIBLE_PADDING_MODE.
     *
     * If the block mode is BlockMode::CBC, BlockMode::CTR, or BlockMode::GCM, an initialization
     * vector or nonce is required.  In most cases, callers shouldn't provide an IV or nonce and the
     * IKeymasterDevice implementation must generate a random IV or nonce and return it via
     * Tag::NONCE in outParams.  CBC and CTR IVs are 16 bytes.  GCM nonces are 12 bytes.  If the key
     * authorizations contain Tag::CALLER_NONCE, then the caller may provide an IV/nonce with
     * Tag::NONCE in inParams.  If a nonce is provided when Tag::CALLER_NONCE is not authorized,
     * begin() must return ErrorCode::CALLER_NONCE_PROHIBITED.  If a nonce is not provided when
     * Tag::CALLER_NONCE is authorized, IKeymasterDevice msut generate a random IV/nonce.
     *
     * -- HMAC keys --
     *
     * HMAC key operations must specify Tag::MAC_LENGTH in inParams.  The specified value must be a
     * multiple of 8 that is not greater than the digest length or less than the value of
     * Tag::MIN_MAC_LENGTH in the key authorizations.  For MAC lengths greater than the digest
     * length or non-multiples of 8, begin() must return ErrorCode::UNSUPPORTED_MAC_LENGTH.  For
     * values less than the key's minimum length, begin() must return ErrorCode::INVALID_MAC_LENGTH.
     *
     * @param purpose The purpose of the operation, one of KeyPurpose::ENCRYPT, KeyPurpose::DECRYPT,
     *        KeyPurpose::SIGN or KeyPurpose::VERIFY.  Note that for AEAD modes, encryption and
     *        decryption imply signing and verification, respectively, but must be specified as
     *        KeyPurpose::ENCRYPT and KeyPurpose::DECRYPT.
     *
     * @param keyBlob The opaque key descriptor returned by generateKey() or importKey().  The key
     *        must have a purpose compatible with purpose and all of its usage requirements must be
     *        satisfied, or begin() must return an appropriate error code (see above).
     *
     * @param inParams Additional parameters for the operation.  If Tag::APPLICATION_ID or
     *        Tag::APPLICATION_DATA were provided during generation, they must be provided here, or
     *        the operation must fail with ErrorCode::INVALID_KEY_BLOB.  For operations that require
     *        a nonce or IV, on keys that were generated with Tag::CALLER_NONCE, inParams may
     *        contain a tag Tag::NONCE.  If Tag::NONCE is provided for a key without
     *        Tag:CALLER_NONCE, ErrorCode::CALLER_NONCE_PROHIBITED must be returned.
     *
     * @param authToken Authentication token.  Callers that provide no token must set all numeric
     *        fields to zero and the MAC must be an empty vector.
     *
     * @return outParams Output parameters.  Used to return additional data from the operation
     *         initialization, notably to return the IV or nonce from operations that generate an IV
     *         or nonce.
     *
     * @return operationHandle The newly-created operation handle which must be passed to update(),
     *         finish() or abort().
     */
    void begin(in KeyPurpose inPurpose, in byte[] inKeyBlob,
               in KeyParameter[] inParams, in KeymasterCapability inCapability,
               out KeyParameter[] outParams, out IKeymasterOperation outOperationHandle);
};
