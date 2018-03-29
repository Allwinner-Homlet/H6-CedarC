LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../../..
include ${SCLIB_TOP}/config.mk

h264_srcs_c   :=  h264.c
h264_srcs_c   +=  h264_dec.c
h264_srcs_c   +=  h264_hal.c
h264_srcs_c   +=  h264_mmco.c
h264_srcs_c   +=  h264_mvc.c
h264_srcs_c   +=  h264_nalu.c

h264_srcs_inc := $(LOCAL_PATH) \
				$(SCLIB_TOP)/include/ \
				$(SCLIB_TOP)/ve/include \
				$(SCLIB_TOP)/base/include/ \
				$(SCLIB_TOP)/vdecoder/include \
				$(SCLIB_TOP)/vdecoder \
				$(SCLIB_TOP)/vdecoder/videoengine \

LOCAL_SRC_FILES := $(h264_srcs_c)
LOCAL_C_INCLUDES := $(h264_srcs_inc)
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
	libvdecoder \
	libvideoengine \
	liblog

#libawavs
LOCAL_MODULE:= libawh264

include $(BUILD_SHARED_LIBRARY)

#==================================
#==== build vendor lib ===========

ifeq ($(COMPILE_SO_IN_VENDOR), yes)

include $(CLEAR_VARS)

SCLIB_TOP=${LOCAL_PATH}/../../..
include ${SCLIB_TOP}/config.mk

h264_srcs_c   :=  h264.c
h264_srcs_c   +=  h264_dec.c
h264_srcs_c   +=  h264_hal.c
h264_srcs_c   +=  h264_mmco.c
h264_srcs_c   +=  h264_mvc.c
h264_srcs_c   +=  h264_nalu.c

h264_srcs_inc := $(LOCAL_PATH) \
				$(SCLIB_TOP)/include/ \
				$(SCLIB_TOP)/ve/include \
				$(SCLIB_TOP)/base/include/ \
				$(SCLIB_TOP)/vdecoder/include \
				$(SCLIB_TOP)/vdecoder \
				$(SCLIB_TOP)/vdecoder/videoengine \

LOCAL_SRC_FILES := $(h264_srcs_c)
LOCAL_C_INCLUDES := $(h264_srcs_inc)
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
	libomx_vdecoder \
	libomx_videoengine \
	liblog

#libawavs
LOCAL_MODULE:= libomx_h264
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

endif
