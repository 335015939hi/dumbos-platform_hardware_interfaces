package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesKey;
import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;

union OpaqueOrAes {
    OpaqueKeyMaterial Opaque;
    AesKey Explicit;
}
