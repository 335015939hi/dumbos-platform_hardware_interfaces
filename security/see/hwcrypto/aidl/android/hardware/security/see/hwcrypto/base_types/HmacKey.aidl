package android.hardware.security.see.hwcrypto.base_types;

// unions require a default value for its first element.
union HmacKey {
    byte[28] HmacSha224 = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    byte[32] HmacSha256;
    byte[48] HmacSha384;
    byte[64] HmacSha512;
}
