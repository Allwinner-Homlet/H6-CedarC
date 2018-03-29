LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../../..
include ${SCLIB_TOP}/config.mk

h265_srcs_c   := h265.c \
				 h265_debug.c  \
				 h265_drv.c  \
				 h265_md5.c  \
				 h265_memory.c  \
				 h265_parser.c  \
				 h265_ref.c  \
				 h265_register.c \
				 h265_sha.c

h265_srcs_inc := 	$(LOCAL_PATH) \
                $(SCLIB_TOP)/include/ \
                $(SCLIB_TOP)/vdecoder/include \
                $(SCLIB_TOP)/vdecoder \
                $(SCLIB_TOP)/ve/include \
                $(SCLIB_TOP)/base/include/ \
                $(SCLIB_TOP)/base/include/gralloc_metadata \
                $(SCLIB_TOP)/vdecoder/videoengine

LOCAL_SRC_FILES := $(h265_srcs_c)
LOCAL_C_INCLUDES := $(h265_srcs_inc)
LOCAL_CFLAGS :=
LOCAL_LDFLAGS :=

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libui       \
	libdl       \
	libVE       \
	libMemAdapter \
	libvdecoder \
	libvideoengine \
	liblog

#libawh265
LOCAL_MODULE:= libawh265

include $(BUILD_SHARED_LIBRARY)

#======================================
#=======build vendor lib ==============

ifeq ($(COMPILE_SO_IN_VENDOR), yes)

include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../../..
include ${SCLIB_TOP}/config.mk

h265_srcs_c   := h265.c \
				 h265_debug.c  \
				 h265_drv.c  \
				 h265_md5.c  \
				 h265_memory.c  \
				 h265_parser.c  \
				 h265_ref.c  \
				 h265_register.c \
				 h265_sha.c

h265_srcs_inc := 	$(LOCAL_PATH) \
                $(SCLIB_TOP)/include/ \
                $(SCLIB_TOP)/vdecoder/include \
                $(SCLIB_TOP)/vdecoder \
                $(SCLIB_TOP)/ve/include \
                $(SCLIB_TOP)/base/include/ \
                $(SCLIB_TOP)/base/include/gralloc_metadata \
                $(SCLIB_TOP)/vdecoder/videoengine

LOCAL_SRC_FILES := $(h265_srcs_c)
LOCAL_C_INCLUDES := $(h265_srcs_inc)
LOCAL_CFLAGS :=
LOCAL_LDFLAGS :=

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libui       \
	libdl       \
	libomx_VE       \
	libomx_MemAdapter \
	libomx_vdecoder \
	libomx_videoengine \
	liblog

#libawh265
LOCAL_MODULE:= libomx_h265
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

endif

