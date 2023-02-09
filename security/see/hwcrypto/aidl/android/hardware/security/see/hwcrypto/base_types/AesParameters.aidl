package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesCipherMode;
import android.hardware.security.see.hwcrypto.base_types.AesGcmMode;

union AesParameters {
    AesGcmMode Gcm;
    AesCipherMode CipherMode;
}
