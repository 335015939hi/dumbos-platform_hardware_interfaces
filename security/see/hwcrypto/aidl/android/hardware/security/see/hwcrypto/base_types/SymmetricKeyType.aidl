package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesKeyType;
import android.hardware.security.see.hwcrypto.base_types.TDesKeyType;

union SymmetricKeyType {
    AesKeyType Aes;
    TDesKeyType TDes;
}
