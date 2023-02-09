package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesMode;
import android.hardware.security.see.hwcrypto.base_types.AesVariant;

parcelable AesKeyType {
    AesMode mode = AesMode.GCM;
    AesVariant variant = AesVariant.AES256;
}
