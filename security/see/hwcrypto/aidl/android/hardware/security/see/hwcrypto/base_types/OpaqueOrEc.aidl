package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EcKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;

union OpaqueOrEc {
    OpaqueKeyMaterial Opaque;
    EcKeyMaterial Explicit;
}
