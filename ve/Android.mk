LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../config.mk
################################################################################
## set flags for golobal compile and link setting.
################################################################################

LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set the include path for compile flags.
LOCAL_SRC_FILES:= veAdapter.c \
                veAw/veAw.c \
                veVp9/veVp9.c

LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/include \
				$(LOCAL_PATH)/../include/ \
				$(LOCAL_PATH)/../base/include/ \

#$(LOCAL_PATH)/../vdecoder/include/

LOCAL_SHARED_LIBRARIES := libcutils libutils libcdc_base liblog

#libve
LOCAL_MODULE:= libVE

include $(BUILD_SHARED_LIBRARY)

#===================================
#========== build vendor lib for omx ===

ifeq ($(COMPILE_SO_IN_VENDOR), yes)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../config.mk
LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set the include path for compile flags.
LOCAL_SRC_FILES:= veAdapter.c \
                veAw/veAw.c \
                veVp9/veVp9.c

LOCAL_C_INCLUDES := \
				$(LOCAL_PATH)/include \
				$(LOCAL_PATH)/../include/ \
				$(LOCAL_PATH)/../base/include/ \

#$(LOCAL_PATH)/../vdecoder/include/

LOCAL_SHARED_LIBRARIES := libcutils libutils libomx_cdc_base liblog

#libve
LOCAL_MODULE:= libomx_VE

include $(BUILD_SHARED_LIBRARY)

endif
