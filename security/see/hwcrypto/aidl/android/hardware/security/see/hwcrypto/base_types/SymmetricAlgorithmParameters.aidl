package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesParameters;
import android.hardware.security.see.hwcrypto.base_types.TDesParameters;

union SymmetricAlgorithmParameters {
    AesParameters Aes;
    TDesParameters TDes;
}
