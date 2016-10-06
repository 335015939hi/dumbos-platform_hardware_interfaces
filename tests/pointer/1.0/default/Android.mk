LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.tests.pointer@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Graph.cpp \
    Pointer.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidl \
    libhwbinder \
    libutils \
    android.hardware.tests.pointer@1.0 \

include $(BUILD_SHARED_LIBRARY)
