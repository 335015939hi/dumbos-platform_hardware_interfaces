package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.IAccumulatingOperation;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union AccumulatingOperationResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    IAccumulatingOperation Ok;
}
