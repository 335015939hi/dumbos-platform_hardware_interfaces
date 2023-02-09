package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HmacVariant;

parcelable HmacKeyType {
    HmacVariant variant = HmacVariant.SHA256;
    int hash_key_size;
}
