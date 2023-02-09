package android.hardware.security.see.hwcrypto.base_types;

// Lifetime values represented as a bitmask. This allow us to combined them on a single property on
// operations to describe a set of allowed lifetimes.
/*
 * enum KeyClass - Gives more information about the characteristics of the key.
 *
 * @EPHEMERAL:
 *      Hardware keys with limited validity (until key is erased or power cycle occurs).
 * @HARDWARE:
 *      Key only lives or was derived from a key that only lives in hardware. This key cannot be
 *      retrieved in the clear.
 * @PORTABLE:
 *      Key could have been at some point of its lifetime in the clear on a software component.
 */
@Backing(type="byte")
enum KeyClass {
    EPHEMERAL = 1,
    HARDWARE = 2,
    PORTABLE = 4,
}
