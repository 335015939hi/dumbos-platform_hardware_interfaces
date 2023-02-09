package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.TDesMode;

parcelable TDesKey {
    TDesMode mode = TDesMode.CBC_PKCS7_PADDING;
    byte[21] key_material;
}
