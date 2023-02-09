package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.TDesOperationMode;

parcelable TDesKey {
    TDesOperationMode mode;
    byte[21] keyMaterial;
}
