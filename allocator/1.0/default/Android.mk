LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.allocator@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Allocator.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    libcutils \
    android.hardware.allocator@1.0 \

include $(BUILD_SHARED_LIBRARY)
