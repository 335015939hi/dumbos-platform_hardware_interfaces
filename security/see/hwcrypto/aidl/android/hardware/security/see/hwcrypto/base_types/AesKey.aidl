package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.AesOperationMode;

parcelable AesKey {
    AesOperationMode mode;
    AesKeyMaterial keyMaterial;
}
