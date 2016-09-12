LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hidl_bluetooth_test
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES :=                                  \
    main.cpp                                        \

LOCAL_SHARED_LIBRARIES :=       	\
    libandroid_runtime          	\
    libbase                     	\
    libcutils                   	\
    libhidl                      	\
    libhwbinder                  	\
    libnativehelper             	\
    libutils                    	\
    android.hardware.bluetooth@1.0	\

LOCAL_STATIC_LIBRARIES := \
    libgtest

LOCAL_MODULE_TAGS := samples

LOCAL_CFLAGS := -O0 -g

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
