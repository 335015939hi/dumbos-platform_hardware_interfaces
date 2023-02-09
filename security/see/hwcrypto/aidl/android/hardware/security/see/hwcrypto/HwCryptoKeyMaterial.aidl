package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;
import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;

/*
 * parcelable HwCryptoKeyMaterial - Format used to represent a cryptographic key. It includes key
 *                                  material (opaque or in the clear) and the policy for its use.
 *
 * @policy:
 *      Parcelable that describes how the key can be used.
 * @oem_data:
 *      OEM specific data.
 * @key_material:
 *      Opaque key material. At this level we do not support explicit key material, because those
 *      operations are handled on a higher level on the library.
 * @mac:
 *      MAC of all fields to provide some level of tampering protection for policy fields or
 *      explicit key material.
 */
parcelable HwCryptoKeyMaterial {
    KeyPolicy policy;
    byte[] oem_data; // TODO: Check use case and if we need this field.
    OpaqueKeyMaterial key_material;
    byte[] mac;
}
