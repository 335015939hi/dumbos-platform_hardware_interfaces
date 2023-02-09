package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EmptyEnum;
import android.hardware.security.see.hwcrypto.base_types.OaepPadding;

union RsaDecryptParameters {
    EmptyEnum NoPadding = EmptyEnum.NONE;
    OaepPadding oaep_padding;
    EmptyEnum Pkcs1_1_5Padding;
}
