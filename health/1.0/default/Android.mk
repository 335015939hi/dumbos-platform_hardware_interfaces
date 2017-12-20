LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.health@1.0-impl
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_C_INCLUDES := system/core/base/include
LOCAL_SRC_FILES := \
    Health.cpp \

LOCAL_HEADER_LIBRARIES := libhealthd_headers

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    android.hardware.health@1.0 \

LOCAL_STATIC_LIBRARIES := android.hardware.health@1.0-convert

LOCAL_HAL_STATIC_LIBRARIES := libhealthd

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
