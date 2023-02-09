package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.Digest;
import android.hardware.security.see.hwcrypto.base_types.EmptyEnum;

union RsaSignMode {
    EmptyEnum NoPadding = EmptyEnum.NONE;
    Digest PssPadding;
    Digest Pkcs1_1_5Padding;
}
