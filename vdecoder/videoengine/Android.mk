LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../..
include ${SCLIB_TOP}/config.mk

current_path := $(LOCAL_PATH)


libvideoengine_inc_common := 	$(current_path) \
                    		$(SCLIB_TOP)/ve/include \
		                    $(SCLIB_TOP)/include \
		                    $(SCLIB_TOP)/base/include \
                                    $(SCLIB_TOP)/base/include/gralloc_metadata \
		                    $(SCLIB_TOP)/vdecoder/include \
		                    $(SCLIB_TOP)/vdecoder \
		                    $(LOCAL_PATH)/include \
		                    $(LOCAL_PATH) \

LOCAL_SRC_FILES := videoengine.c
LOCAL_C_INCLUDES := $(libvideoengine_inc_common)
LOCAL_CFLAGS :=
LOCAL_LDFLAGS :=


LOCAL_MODULE_TAGS := optional

## add libaw* for eng/user rebuild
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libui       \
	libdl       \
	libVE       \
	liblog


LOCAL_MODULE := libvideoengine

include $(BUILD_SHARED_LIBRARY)

#=================================================
#======== build vendor lib ======================

ifeq ($(COMPILE_SO_IN_VENDOR), yes)

include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../..
include ${SCLIB_TOP}/config.mk

current_path := $(LOCAL_PATH)


libvideoengine_inc_common := 	$(current_path) \
                    		$(SCLIB_TOP)/ve/include \
		                    $(SCLIB_TOP)/include \
		                    $(SCLIB_TOP)/base/include \
                                    $(SCLIB_TOP)/base/include/gralloc_metadata \
		                    $(SCLIB_TOP)/vdecoder/include \
		                    $(SCLIB_TOP)/vdecoder \
		                    $(LOCAL_PATH)/include \
		                    $(LOCAL_PATH) \

LOCAL_SRC_FILES := videoengine.c
LOCAL_C_INCLUDES := $(libvideoengine_inc_common)
LOCAL_CFLAGS :=
LOCAL_LDFLAGS :=


LOCAL_MODULE_TAGS := optional

## add libaw* for eng/user rebuild
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libui       \
	libdl       \
	libomx_VE       \
	liblog

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libomx_videoengine

include $(BUILD_SHARED_LIBRARY)

endif

include $(call all-makefiles-under,$(LOCAL_PATH))
