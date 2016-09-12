LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.bluetooth@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    BluetoothHci.cpp \
    BluetoothHciEvent.cpp \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libhidl \
    libhwbinder \
    libutils \
    android.hardware.bluetooth@1.0 \

include $(BUILD_SHARED_LIBRARY)
