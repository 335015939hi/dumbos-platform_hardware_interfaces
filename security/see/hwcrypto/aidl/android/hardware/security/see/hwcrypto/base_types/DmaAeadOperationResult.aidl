package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.IDmaAeadOperation;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union DmaAeadOperationResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    IDmaAeadOperation Ok;
}
