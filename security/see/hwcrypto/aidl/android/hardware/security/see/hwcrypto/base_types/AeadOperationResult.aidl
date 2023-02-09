package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.IAeadOperation;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union AeadOperationResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    IAeadOperation Ok;
}
