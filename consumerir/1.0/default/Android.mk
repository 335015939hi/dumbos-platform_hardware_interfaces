LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.consumerir@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Consumerir.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libhardware \
    libhwbinder \
    liblog \
    libutils \
    android.hardware.consumerir@1.0 \

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := android.hardware.consumerir@1.0-service
LOCAL_INIT_RC := android.hardware.consumerir@1.0-service.rc
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhwbinder \
    libhardware \
    libhidlbase \
    libhidltransport \
    libutils \
    android.hardware.consumerir@1.0 \

include $(BUILD_EXECUTABLE)
