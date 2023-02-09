package android.hardware.security.see.hwcrypto.base_types;

// TODO: Add missing combination of uses for combinations that make sense
// Represented as bitmask because these can be mixed
@Backing(type="int")
enum KeyUse {
    ENCRYPT = 1,
    DECRYPT = 2,
    SIGN = 4,
    VERIFY = 8,
    EXCHANGE = 16,
    DERIVE = 32,
    WRAP = 64,
    UNSPECIFIED = 128, // Combination of all uses
}
