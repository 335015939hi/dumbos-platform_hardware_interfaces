package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.Domain;
import android.hardware.security.see.hwcrypto.base_types.EvictReason;
import android.hardware.security.see.hwcrypto.base_types.KeyClass;
import android.hardware.security.see.hwcrypto.base_types.KeyPermissions;
import android.hardware.security.see.hwcrypto.base_types.KeyUse;

/*
 * parcelable KeyPolicy - Structure that specified how a key can be used.
 *
 * @usage:
 *      Enum specifying the operations the key can perform (encryption, decryption, etc.).
 * @key_class:
 *      Enum that describes the key type. See the docstring on %KeyClass for more details.
 * @evict_policy:
 *      Policy to invalidate the key. See the docstring on %EvictReason for more details.
 * @key_permissions:
 *      Additional permissions of the key (e.g. key types allowed to wrap the key, boot binsing,
 *      etc.). See the docstring on %KeyPermissions for more details.
 * @owner:
 *      Domain that controls the key.
 * @authorized_domains:
 *      Domains that can use the key.
 */
parcelable KeyPolicy {
    KeyUse usage = KeyUse.UNSPECIFIED; // TODO: check if using a vector would be preferable
    KeyClass key_class = KeyClass.EPHEMERAL;
    EvictReason[] evict_policy;
    KeyPermissions[] key_permissions;
    Domain owner;
    Domain[] authorized_domains;
}
