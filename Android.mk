LOCAL_PATH := $(call my-dir)

# Install all compatibility_matrix.*.xml to /system/etc/vintf

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.legacy.xml
LOCAL_MODULE_STEM := compatibility_matrix.legacy.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.1.xml
LOCAL_MODULE_STEM := compatibility_matrix.1.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.2.xml
LOCAL_MODULE_STEM := compatibility_matrix.2.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.current.xml
LOCAL_MODULE_STEM := compatibility_matrix.current.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf
include $(BUILD_PREBUILT)

# Framework Compatibility Matrix without HALs
include $(CLEAR_VARS)
LOCAL_MODULE        := framework_compatibility_matrix.empty.xml
LOCAL_MODULE_STEM   := compatibility_matrix.empty.xml
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT)/etc/vintf

GEN := $(local-generated-sources-dir)/$(LOCAL_MODULE_STEM)

$(GEN): PRIVATE_FLAGS :=

ifeq (true,$(BOARD_AVB_ENABLE))
$(GEN): $(AVBTOOL)
# INTERNAL_AVB_SYSTEM_SIGNING_ARGS consists of BOARD_AVB_SYSTEM_KEY_PATH and
# BOARD_AVB_SYSTEM_ALGORITHM. We should add the dependency of key path, which
# is a file, here.
$(GEN): $(BOARD_AVB_SYSTEM_KEY_PATH)
# Use deferred assignment (=) instead of immediate assignment (:=).
# Otherwise, cannot get INTERNAL_AVB_SYSTEM_SIGNING_ARGS.
$(GEN): FRAMEWORK_VBMETA_VERSION = $$("$(AVBTOOL)" add_hashtree_footer \
                           --print_required_libavb_version \
                           $(INTERNAL_AVB_SYSTEM_SIGNING_ARGS) \
                           $(BOARD_AVB_SYSTEM_ADD_HASHTREE_FOOTER_ARGS))
else
$(GEN): FRAMEWORK_VBMETA_VERSION := 0.0
endif

# Specify kernel versions that the current framework supports. These versions,
# along with kernel configurations, are written to the framework compatibility
# matrix.
$(GEN): KERNEL_VERSIONS := 3.18 4.4 4.9

# Specify the location of android-base*.cfg files.
$(GEN): KERNEL_CONFIG_DATA := kernel/configs

$(GEN): $(foreach version,$(KERNEL_VERSIONS),\
	$(wildcard $(KERNEL_CONFIG_DATA)/android-$(version)/android-base*.cfg))
$(GEN): PRIVATE_FLAGS += $(foreach version,$(KERNEL_VERSIONS),\
	--kernel=$(version):$(call normalize-path-list,\
		$(wildcard $(KERNEL_CONFIG_DATA)/android-$(version)/android-base*.cfg)))

$(GEN): $(LOCAL_PATH)/compatibility_matrix.empty.xml $(HOST_OUT_EXECUTABLES)/assemble_vintf
	POLICYVERS=$(POLICYVERS) \
		BOARD_SEPOLICY_VERS=$(BOARD_SEPOLICY_VERS) \
		FRAMEWORK_VBMETA_VERSION=$(FRAMEWORK_VBMETA_VERSION) \
		PRODUCT_ENFORCE_VINTF_MANIFEST=$(PRODUCT_ENFORCE_VINTF_MANIFEST) \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf \
		-i $< -o $@ $(PRIVATE_FLAGS)
LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix_sources
LOCAL_REQUIRED_MODULES := \
    framework_compatibility_matrix.legacy.xml \
    framework_compatibility_matrix.1.xml \
    framework_compatibility_matrix.2.xml \
    framework_compatibility_matrix.current.xml \
    framework_compatibility_matrix.empty.xml

FRAMEWORK_COMPATIBILITY_MATRIX_SRC_FILES := \
	$(call module-installed-files,$(LOCAL_REQUIRED_MODULES))
include $(BUILD_PHONY_PACKAGE)

include $(call first-makefiles-under,$(LOCAL_PATH))
