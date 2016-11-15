LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.dumpstate@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Dumper.cpp \
    DumpstateDevice.cpp \

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidl \
    libhwbinder \
    liblog \
    libutils \
    android.hardware.dumpstate@1.0 \

include $(BUILD_SHARED_LIBRARY)
