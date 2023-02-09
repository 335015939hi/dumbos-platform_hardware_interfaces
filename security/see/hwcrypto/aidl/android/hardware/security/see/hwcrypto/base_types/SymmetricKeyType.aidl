package android.hardware.security.see.hwcrypto.base_types;

// New values ahould be set to match PSA spec when possible
// bit 14: Assymmetric key; set to 0 (setting it to 1 indicates an asymmetric key)
// bits 13 and 12: Key type; set to 10 (symmetric key) for AES and DES. HMAC keys are considered RAW
//                 on this spec, so following its convention we set this to 01.
// bits 11-1 :  Key type dependant (See PSA spec).
// bit 0: Parity bit, valid encodings have even parity
@Backing(type="int")
enum SymmetricKeyType {
    HmacKey = 0x5000,
    TripleDesKey = 0x6301,
    AesKey = 0x6401
}
