package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union U64Result {
    HalErrorCode Err = HalErrorCode.GENERIC_ERROR;
    long Ok; // TODO: long is not really unsigned
}
