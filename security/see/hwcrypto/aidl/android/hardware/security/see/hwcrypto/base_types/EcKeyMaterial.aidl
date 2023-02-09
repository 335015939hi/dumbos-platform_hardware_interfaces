package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.Ed25519Key;
import android.hardware.security.see.hwcrypto.base_types.NistKey;
import android.hardware.security.see.hwcrypto.base_types.X25519Key;

union EcKeyMaterial {
    NistKey P224;
    NistKey P256;
    NistKey P384;
    NistKey P512;
    Ed25519Key Ed25519;
    X25519Key X25519;
}
