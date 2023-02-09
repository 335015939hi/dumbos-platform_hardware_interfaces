package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.HwCryptoKeyResult;
import android.hardware.security.see.hwcrypto.base_types.KdfVersion;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;
import android.hardware.security.see.hwcrypto.base_types.SymmetricKeyType;

interface HwCryptoKey {
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
     *     version of the KDF to use; can be set to None.
     * @batch_key:
     *     if true, the derived key will be consistent and shared across the entire
     *     family of devices, given the same input (shared key). If false, the derived key will
     *     be unique to the particular device it was derived on.
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
    HwCryptoKeyResult hwkey_derive_versioned(
            /*@nullable*/ KdfVersion
                    kdf_version, // TODO @nullable cannot be used on enums
            boolean batch_key,
            /*@nullable*/ SymmetricKeyType key_type, // TODO: @nullable is not
                                                             // allowed for enums
            KeyVersionSource rollback_version_source, inout ComponentVersion[] rollback_versions,
            in byte[] context);
}
