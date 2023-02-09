package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesCipherModeParameters;
import android.hardware.security.see.hwcrypto.base_types.EmptyEnum;

union AesCipherMode {
    EmptyEnum EcbNoPadding = EmptyEnum.NONE;
    AesCipherModeParameters EcbPkcs7PAdding;
    AesCipherModeParameters CbcNoPadding;
    AesCipherModeParameters CbcPkcs7PAdding;
    AesCipherModeParameters Ctr;
}
