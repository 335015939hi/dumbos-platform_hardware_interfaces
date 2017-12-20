LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := android.hardware.combinedhals-service
LOCAL_INIT_RC := android.hardware.combinedhals-service.rc
LOCAL_SRC_FILES := \
    service.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhardware \

LOCAL_SHARED_LIBRARIES += \
    libhidlbase \
    libhidltransport \
    android.hardware.light@2.0 \
    android.hardware.vibrator@1.0 \
    android.hardware.health@1.0 \
    android.hardware.memtrack@1.0 \

include $(BUILD_EXECUTABLE)
