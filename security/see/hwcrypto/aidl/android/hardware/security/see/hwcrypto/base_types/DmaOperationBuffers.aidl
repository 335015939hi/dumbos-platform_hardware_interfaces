package android.hardware.security.see.hwcrypto.base_types;

import android.hardware.security.see.hwcrypto.base_types.DmaBufferArea;

parcelable DmaOperationBuffers {
    @nullable DmaBufferArea input;
    @nullable DmaBufferArea output;
}
