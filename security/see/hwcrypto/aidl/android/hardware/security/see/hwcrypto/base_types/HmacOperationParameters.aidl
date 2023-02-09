package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.Digest;

parcelable HmacOperationParameters {
    Digest digest = Digest.SHA256;
}
