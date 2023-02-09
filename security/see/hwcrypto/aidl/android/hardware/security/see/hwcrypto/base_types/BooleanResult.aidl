package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union BooleanResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    boolean Ok;
}
