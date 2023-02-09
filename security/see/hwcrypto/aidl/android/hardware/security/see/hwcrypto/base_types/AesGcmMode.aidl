package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesGcmModeParameters;

union AesGcmMode {
    AesGcmModeParameters GcmTag12;
    AesGcmModeParameters GcmTag13;
    AesGcmModeParameters GcmTag14;
    AesGcmModeParameters GcmTag15;
    AesGcmModeParameters GcmTag16;
}
