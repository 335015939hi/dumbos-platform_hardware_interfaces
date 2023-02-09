package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.base_types.AeadOperationResult;
import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.DmaOperationBuffers;
import android.hardware.security.see.hwcrypto.base_types.EmittingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.HwCryptoKeyResult;
import android.hardware.security.see.hwcrypto.base_types.KeyType;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;
import android.hardware.security.see.hwcrypto.base_types.NullableKdfVersion;
import android.hardware.security.see.hwcrypto.base_types.SymmetricKeyType;
import android.hardware.security.see.hwcrypto.base_types.SymmetricOperationParameters;

// TODO: @nullable cannot be used on enums, so using wrappers for them for now

interface IHwCryptoKeyOperations {
    /*
     * get_keyslot_data() - Gets the keyslot key material referenced by slot_id.
     *
     * @slot_id:
     *      string identifier for the requested keyslot
     *
     * Because this access a shared key, the identity of the caller needs to be checked
     * to verify that it has permission to access the requested key.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - IoError: if there's an issue communicating with the service
     *          - NotFound: if keyslot is not found
     */
    HwCryptoKeyResult get_keyslot_data(String slot_id);

    /*
     * hwkey_derive_versioned() - Derive a versioned, device-specific or batch key from
     *                            provided context.
     *
     * @kdf_version:
     *     version of the KDF to use; can be set to Null to use default value.
     * @batch_key:
     *     if true, the derived key will be consistent and shared across the entire
     *     family of devices, given the same input (shared key). If false, the derived key will
     *     be unique to the particular device it was derived on.
     * @key_type:
     *     desired type of key to be generated (AES, etc.). Can be set to Null to use default value.
     * @rollback_version_source:
     *     specifies whether the @rollback_versions must have been committed. If
     *     %CommittedVersion is specified, the system must guarantee
     *     that software with a lower rollback version cannot ever run on a future
     *     boot. (see &enum KeyVersionSource)
     * @rollback_versions:
     *     (in/out) the different components (OS, etc.) rollback versions to be incorporated into
     *     the key derivation. Must be less than or equal to the current component rollback
     *     version from @rollback_version_source. If a component version isn't set, the latest
     *     available version will be used. After execution, the versions used for the key derivation
     *     will be present on this argument
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - NotValid - invalid parameters
     *          - NotImplemented - the requested version source or KDF mode is not supported
     */
    HwCryptoKeyResult hwkey_derive_versioned(in @nullable NullableKdfVersion kdf_version,
            boolean batch_key, in @nullable KeyType key_type,
            KeyVersionSource rollback_version_source,
            in @nullable ComponentVersion[] rollback_versions, in byte[] context);

    /*
     * import_key_into_domain() - Imports a key into a different domain. This allows the use of a
     *                            SW created key on a hardware block.
     *
     * @key_to_be_imported:
     *     key to be imported into a new domain
     * @new_key_policy:
     *      Policy of the new key. Defines how the new key can be used..
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult import_key_into_domain(
            in HwCryptoKeyMaterial key_to_be_imported, in KeyPolicy new_key_policy);

    /*
     * derive_key() - Derives a new key.
     *
     * @policy:
     *      Policy of the new key. Defines how the new key can be used.
     * @key_type:
     *     desired type of key to be generated (AES, etc.).
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult generate_key(in KeyPolicy policy, in KeyType key_type);

    /*
     * derive_key() - Derives a new key.
     *
     * @derivation_key:
     *      Key that will be used to create a new Key. Its type and policy needs to be compatible
     *      with deriving keys out of it.
     * @policy:
     *      Policy of the derived key. Defines how the new key can be used.
     * @key_type:
     *     desired type of key to be generated (AES, etc.).
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult derive_key(in HwCryptoKeyMaterial derivation_key, in KeyPolicy policy,
            in KeyType key_type, in byte[] context);

    /*
     * secure_import_key_into_engine() - Creates a secure channel with a server to import a key.
     *                            (TODO: complete definition).
     */

    /*
     * begin_symmetric_operation() - start a symmetric cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    EmittingOperationResult begin_symmetric_operation(
            in HwCryptoKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_symmetric_operation_aead() - start an authenticated encryption with additional data
     *                                    cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IAeadOperation) on success, specific error code on error.
     */
    AeadOperationResult begin_symmetric_operation_aead(
            in HwCryptoKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_symmetric_operation_dma() - start a symmetric cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     * @dma_buffers:
     *      buffers to be used for the operation
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    EmittingOperationResult begin_symmetric_operation_dma(in HwCryptoKeyMaterial key,
            in SymmetricOperationParameters parameters, in DmaOperationBuffers dma_buffers);

    /*
     * begin_symmetric_operation_dma_aead() - start an authenticated encryption with additional data
     *                                        cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     * @dma_buffers:
     *      buffers to be used for the operation
     *
     * Return:
     *      Ok(IAeadOperation) on success, specific error code on error.
     */
    AeadOperationResult begin_symmetric_operation_dma_aead(in HwCryptoKeyMaterial key,
            in SymmetricOperationParameters parameters, in DmaOperationBuffers dma_buffers);
}
