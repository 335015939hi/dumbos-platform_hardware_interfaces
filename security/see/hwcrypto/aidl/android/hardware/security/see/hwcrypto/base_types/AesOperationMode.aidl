package android.hardware.security.see.hwcrypto.base_types;

@Backing(type="byte")
enum AesOperationMode {
    Ecb,
    Cbc,
    Ctr,
    Gcm,
    Xts,
    Cmac,
    Kw,
}
