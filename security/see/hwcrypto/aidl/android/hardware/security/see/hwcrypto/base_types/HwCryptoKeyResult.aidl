package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

union HwCryptoKeyResult {
    HwCryptoKeyMaterial Ok;
    HalErrorCode Err;
}
