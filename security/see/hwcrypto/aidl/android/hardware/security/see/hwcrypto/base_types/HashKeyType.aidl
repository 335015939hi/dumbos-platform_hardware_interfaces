package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.HmacKeyType;

union HashKeyType {
    HmacKeyType Sha2;
    HmacKeyType Sha3;
}
