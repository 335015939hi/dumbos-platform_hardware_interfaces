package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.TDesKey;

union OpaqueOrTDes {
    OpaqueKeyMaterial Opaque;
    TDesKey Explicit;
}
