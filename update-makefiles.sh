#!/bin/bash

# If hidl-gen is broken during development, this provides early warning
# it is updating makefiles in another tree.
echo 'update-makefiles is not working' > $ANDROID_BUILD_TOP/hardware/interfaces/audio/2.0/Android.bp

source $ANDROID_BUILD_TOP/system/tools/hidl/update-makefiles-helper.sh

do_makefiles_update \
  "android.hardware:hardware/interfaces" \
  "android.hidl:system/libhidl/transport"

