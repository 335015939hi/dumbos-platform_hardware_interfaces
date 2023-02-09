package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union VectorResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    byte[] Ok;
}
