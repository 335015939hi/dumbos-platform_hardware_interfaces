#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

##### Input Variables:
# LOCAL_MODULE: required. Module name for the build system.
# LOCAL_MODULE_CLASS: optional. Default is ETC.
# LOCAL_MODULE_PATH / LOCAL_MODULE_RELATIVE_PATH: required. (Relative) path of output file.
#       If not defined, LOCAL_MODULE_RELATIVE_PATH will be "vintf".
# LOCAL_MODULE_STEM: optional. Name of output file. Default is $(LOCAL_MODULE).
# LOCAL_SRC_FILES: required. Local source files provided to assemble_vintf
#       (command line argument -i).
# LOCAL_GENERATED_SOURCES: optional. Global source files provided to assemble_vintf
#       (command line argument -i).
#
# LOCAL_ADD_VBMETA_VERSION: Use AVBTOOL to add avb version to the output matrix
#       (corresponds to <avb><vbmeta-version> tag)
# LOCAL_ASSEMBLE_VINTF_ENV_VARS: Add a list of environment variable names from global variables in
#       the build system that is lazily evaluated (e.g. PRODUCT_ENFORCE_VINTF_MANIFEST).
# LOCAL_ASSEMBLE_VINTF_FLAGS: Add additional command line arguments to assemble_vintf invocation.
# LOCAL_GEN_FILE_DEPENDENCIES: A list of additional dependencies for the generated file.

ifndef LOCAL_MODULE
$(error LOCAL_MODULE must be defined.)
endif

ifndef LOCAL_MODULE_STEM
LOCAL_MODULE_STEM := $(LOCAL_MODULE)
endif

ifndef LOCAL_MODULE_CLASS
LOCAL_MODULE_CLASS := ETC
endif

ifndef LOCAL_MODULE_PATH
ifndef LOCAL_MODULE_RELATIVE_PATH
$(error Either LOCAL_MODULE_PATH or LOCAL_MODULE_RELATIVE_PATH must be defined.)
endif
endif

GEN := $(local-generated-sources-dir)/$(LOCAL_MODULE_STEM)

$(GEN): PRIVATE_ENV_VARS := $(LOCAL_ASSEMBLE_VINTF_ENV_VARS)
$(GEN): PRIVATE_FLAGS := $(LOCAL_ASSEMBLE_VINTF_FLAGS)

$(GEN): $(LOCAL_GEN_FILE_DEPENDENCIES)

ifeq (true,$(strip $(LOCAL_ADD_VBMETA_VERSION)))
ifeq (true,$(BOARD_AVB_ENABLE))
$(GEN): $(AVBTOOL)
$(GEN): $(BOARD_AVB_SYSTEM_KEY_PATH)
# Use deferred assignment (=) instead of immediate assignment (:=).
# Otherwise, cannot get INTERNAL_AVB_SYSTEM_SIGNING_ARGS.
$(GEN): FRAMEWORK_VBMETA_VERSION = $$("$(AVBTOOL)" add_hashtree_footer \
                           --print_required_libavb_version \
                           $(BOARD_AVB_SYSTEM_ADD_HASHTREE_FOOTER_ARGS))
else
$(GEN): FRAMEWORK_VBMETA_VERSION := 0.0
endif # BOARD_AVB_ENABLE
$(GEN): PRIVATE_ENV_VARS += FRAMEWORK_VBMETA_VERSION
endif # LOCAL_ADD_VBMETA_VERSION

my_matrix_src_files := \
	$(addprefix $(LOCAL_PATH)/,$(LOCAL_SRC_FILES)) \
	$(LOCAL_GENERATED_SOURCES)

$(GEN): PRIVATE_SRC_FILES := $(my_matrix_src_files)
$(GEN): $(my_matrix_src_files) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	$(foreach varname,$(PRIVATE_ENV_VARS),$(varname)="$($(varname))") \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf \
		-i $(call normalize-path-list,$(PRIVATE_SRC_FILES)) \
		-o $@ \
		$(PRIVATE_FLAGS)

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
LOCAL_SRC_FILES :=
LOCAL_GENERATED_SOURCES :=

include $(LOCAL_PATH)/clear_vars.mk
my_matrix_src_files :=

include $(BUILD_PREBUILT)
