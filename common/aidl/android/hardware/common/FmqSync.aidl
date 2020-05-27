package android.hardware.common;

import android.hardware.common.NativeHandle;

@VintfStability
parcelable MQDescriptor {
    /**
     * Identifier of the media resource (eg. Drm session id).
     */
    byte[] id;
    
    NativeHandle mHandle;
}
