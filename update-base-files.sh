#!/bin/bash

if [ ! -d $ANDROID_BUILD_TOP/hardware/interfaces ] ; then
  echo "Where is hardware/interfaces?";
  exit 1;
fi

if [ ! -d $ANDROID_BUILD_TOP/system/libhidl/transport ] ; then
  echo "Where is system/libhidl/transport?";
  exit 1;
fi

echo "Changing files in:"

# These files only exist to facilitate the easy transition to hidl.

options="-Lexport-header \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport"

echo hardware/libhardware
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/hardware/libhardware/include_all/hardware/sensors-base.h \
         android.hardware.sensors@1.0
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/hardware/libhardware/include_all/hardware/nfc-base.h \
         android.hardware.nfc@1.0
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/hardware/libhardware/include_all/hardware/gnss-base.h \
         android.hardware.gnss@1.0

echo system/core
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/system/core/include/system/graphics-base-v1.0.h \
         android.hardware.graphics.common@1.0
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/system/core/include/system/graphics-base-v1.1.h \
         android.hardware.graphics.common@1.1

echo system/media
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/system/media/audio/include/system/audio-base.h \
         android.hardware.audio.common@2.0
hidl-gen $options \
         -o $ANDROID_BUILD_TOP/system/media/audio/include/system/audio_effect-base.h \
         android.hardware.audio.effect@2.0
