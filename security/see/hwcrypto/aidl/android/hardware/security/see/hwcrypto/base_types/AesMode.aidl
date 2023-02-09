package android.hardware.security.see.hwcrypto.base_types;

@Backing(type="byte")
enum AesMode {
    ECB_NO_PADDING,
    ECB_PKCS7_PADDING,
    CBC_NO_PADDING,
    CBC_PKCS7_PADDING,
    CTR,
    CMAC,
    GCM,
    XTS,
    KEY_WRAP
}
