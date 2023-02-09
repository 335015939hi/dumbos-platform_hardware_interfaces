package android.hardware.security.see.hwcrypto.base_types;

union RsaKey {
    byte[] Rsa2048 = {};
    byte[] Rsa3072;
    byte[] Rsa4096;
}
