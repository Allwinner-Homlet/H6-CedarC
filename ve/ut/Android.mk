LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

CEDARC = $(TOP)/frameworks/av/media/libcedarc/
################################################################################
## set flags for golobal compile and link setting.
################################################################################

CONFIG_FOR_COMPILE = 
CONFIG_FOR_LINK = 

LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set the include path for compile flags.
LOCAL_SRC_FILES:= $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(SourcePath)                             \
                    $(LOCAL_PATH)/../include \
                    $(LOCAL_PATH)/../common/                      \
                    $(CEDARC)/ve/include                      \
                    $(CEDARC)/include/                        \

LOCAL_SHARED_LIBRARIES := \
            libcutils       \
            libutils        \
            libVE           

#LOCAL_32_BIT_ONLY := true
LOCAL_MODULE:= ut_ve

include $(BUILD_EXECUTABLE)
