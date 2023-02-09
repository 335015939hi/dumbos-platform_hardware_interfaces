package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.KeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;

/*
 * parcelable HwCryptoKeyMaterial - Format used to represent a cryptographic key. It includes key
 *                                  material (opaque or in the clear) and the policy for its use.
 *
 * @policy:
 *      Parcelable that describes how the key can be used.
 * @oem_data:
 *      OEM specific data.
 * @key_material:
 *      Explicit or opaque key material. Explicit key material contains the key in the
 *      clear, while opaque data can only be manipulated by the entity that created the key.
 * @mac:
 *      MAC of all fields to provide some level of tampering protection for policy fields or
 *      explicit key material.
 */
parcelable HwCryptoKeyMaterial {
    KeyPolicy policy;
    byte[] oem_data; // TODO: Check use case and if we need this field.
    KeyMaterial key_material;
    byte[] mac;
}
