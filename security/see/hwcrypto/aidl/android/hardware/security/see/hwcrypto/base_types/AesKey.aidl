package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.AesMode;

parcelable AesKey {
    AesMode mode = AesMode.GCM;
    AesKeyMaterial key_material;
}
