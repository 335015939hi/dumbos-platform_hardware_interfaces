package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.KeyLifetime;
import android.hardware.security.see.hwcrypto.base_types.KeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.KeyOrigin;
import android.hardware.security.see.hwcrypto.base_types.KeyUse;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;

/*
 * parcelable HwCryptoKeyMaterial - Format used to represent a cryptographic key. It includes key
 *                                  material (opaque or in the clear) and the policy for its use.
 *
 * @origin:
 *      Enum specifying the creator of this key. If the key is opaque; this is the only entity
 *      that can use it.
 * @usage:
 *      Enum of the allowed uses for the key (encrypt, verify, etc.)
 * @KeyMaterial:
 *      Explicit or opaque key material. Explicit key material contains the key in the
 *      clear, while opaque data can only be manipulated by the entity that created the key.
 * @oemData:
 *      OEM specific data.
 * @keyMac:
 *      MAC of all fields to provide some level of tampering protection for policy fields or
 *      explicit key material.
 */
parcelable HwCryptoKeyMaterial {
    KeyOrigin origin;
    KeyUse usage;
    byte[] oemData;
    KeyMaterial KeyMaterial;
    byte[] keyMac;
}
