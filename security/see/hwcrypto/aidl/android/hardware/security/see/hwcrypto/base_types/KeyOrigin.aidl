package android.hardware.security.see.hwcrypto.base_types;

/*
 * enum KeyOrigin - Represents the creator of the key. When keys are opaque, this is the entity that
 *                  understand its encoding and/or can retrieve the keys and use them.
 */
@Backing(type="byte")
enum KeyOrigin {
    GeneralPurposeCryptoAcelerator,
    TrustedExecutionEnvironment,
    SoftwareLibrary,
}
