package android.hardware.security.see.hwcrypto.base_types;

// TODO: Add missing combination of uses for combinations that make sense
// TODO: using an int instead of a byte because bytes are signed, cehck if there is a different
//       option
// Represented as bitmask because these can be mixed
@Backing(type="int")
enum KeyUse {
    Encrypt = 1,
    Decrypt = 2,
    Sign = 4,
    Verify = 8,
    Exchange = 16,
    Derive = 32,
    Wrap = 64,
    Unspecified = 128, // Combination of all uses
}
