package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.SymmetricAlgorithmParameters;
import android.hardware.security.see.hwcrypto.base_types.SymmetricOperation;

parcelable SymmetricOperationParameters {
    SymmetricOperation direction = SymmetricOperation.ENCRYPT;
    SymmetricAlgorithmParameters parameters;
}
