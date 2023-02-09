package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union U64Result {
    long Ok; // TODO: long is not really unsigned
    HalErrorCode Err;
}
