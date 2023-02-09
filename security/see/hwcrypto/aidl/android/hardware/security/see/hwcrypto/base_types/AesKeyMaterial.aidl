package android.hardware.security.see.hwcrypto.base_types;

// unions require a default value for its first element.
union AesKeyMaterial {
    byte[16] Aes128 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    byte[24] Aes192;
    byte[32] Aes256;
}
