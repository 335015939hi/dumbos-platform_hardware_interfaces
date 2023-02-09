package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.RsaKey;

union OpaqueOrRsa {
    OpaqueKeyMaterial Opaque;
    RsaKey Explicit;
}
