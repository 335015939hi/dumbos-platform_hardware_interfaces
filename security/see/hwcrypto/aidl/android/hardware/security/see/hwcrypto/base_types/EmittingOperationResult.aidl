package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union EmittingOperationResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    IEmittingOperation Ok;
}
