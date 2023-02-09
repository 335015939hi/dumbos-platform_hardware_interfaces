package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HmacKey;
import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;

union OpaqueOrHmac {
    OpaqueKeyMaterial Opaque;
    HmacKey Explicit;
}
