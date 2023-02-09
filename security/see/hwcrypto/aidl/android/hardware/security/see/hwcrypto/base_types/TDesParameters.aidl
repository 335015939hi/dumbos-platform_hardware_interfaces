package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.EmptyEnum;
import android.hardware.security.see.hwcrypto.base_types.TDesModeParameters;

union TDesParameters {
    EmptyEnum EcbNoPadding = EmptyEnum.NONE;
    TDesModeParameters EcbPkcs7Padding;
    TDesModeParameters CbcNoPadding;
    TDesModeParameters CbcPkcs7Padding;
}
