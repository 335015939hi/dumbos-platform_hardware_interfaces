package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.AesCipherModeParameters;

// Ecb Mode do not use the AesCipherModeParameters, but AIDL enums/unions do not
// accept elements with no types.
union AesCipherMode {
    AesCipherModeParameters EcbNoPadding;
    AesCipherModeParameters EcbPkcs7PAdding;
    AesCipherModeParameters CbcNoPadding;
    AesCipherModeParameters CbcPkcs7PAdding;
    AesCipherModeParameters Ctr;
}
