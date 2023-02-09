package android.hardware.security.see.hwcrypto.base_types;

enum NonVmDomain {
    HW_ROOT_OF_TRUST, // GROOT
    SECURITY_ANCHOR, // GSA
    SECURE_ENCLAVE, // TZ
    ANDROID_HOST,
}
