package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.IDmaEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union DmaEmittingOperationResult {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    IDmaEmittingOperation Ok;
}
