LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

MOD_PATH=$(LOCAL_PATH)/..
include $(MOD_PATH)/config.mk

################################################################################
## set flags for golobal compile and link setting.
################################################################################

CONFIG_FOR_COMPILE =
CONFIG_FOR_LINK =
LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set source file and include path.
LOCAL_SRC_FILES  := memoryAdapter.c \
                    ionMemory/ionAlloc.c


LOCAL_C_INCLUDES := $(MOD_PATH) \
					$(MOD_PATH)/include \
					$(MOD_PATH)/base/include  \
					$(MOD_PATH)/ve/include

LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libVE libcdc_base


LOCAL_CFLAGS += $(LAW_CFLAGS)


##communicate proxy on client side.
ifeq ($(PLATFORM_SURPPORT_SECURE_OS), yes)

ifeq ($(SECURE_OS_OPTEE), yes)
    LOCAL_SHARED_LIBRARIES+= \
	    libteec

    LOCAL_C_INCLUDES += \
        $(TOP)/hardware/aw/optee_client-master/public
else
    LOCAL_SHARED_LIBRARIES+=\
        libtee_client
    LOCAL_C_INCLUDES += \
    	$(TOP)/hardware/aw/client-api
endif

endif #end# 'PLATFORM_SURPPORT_SECURE_OS == yes'

## compile libMemAdapter
LOCAL_MODULE:= libMemAdapter

ifeq ($(CONFIG_COMPILE_STATIC_LIB), y)
    include $(BUILD_STATIC_LIBRARY)
else
    include $(BUILD_SHARED_LIBRARY)
endif

#========================================
# ========== build vendor lib for omx ===

ifeq ($(COMPILE_SO_IN_VENDOR), yes)

include $(CLEAR_VARS)

MOD_PATH=$(LOCAL_PATH)/..
include $(MOD_PATH)/config.mk
LOCAL_PROPRIETARY_MODULE := true

CONFIG_FOR_COMPILE =
CONFIG_FOR_LINK =
LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set source file and include path.
LOCAL_SRC_FILES  := memoryAdapter.c \
                    ionMemory/ionAlloc.c

ifeq ($(PLATFORM_SURPPORT_SECURE_OS), yes)
LOCAL_SRC_FILES += secureMemory/secureAlloc.c
endif

LOCAL_C_INCLUDES := $(MOD_PATH) \
					$(MOD_PATH)/include \
					$(MOD_PATH)/base/include  \
					$(MOD_PATH)/ve/include

LOCAL_SHARED_LIBRARIES := libcutils libutils liblog libomx_VE libomx_cdc_base

LOCAL_CFLAGS += $(LAW_CFLAGS)

##communicate proxy on client side.
ifeq ($(PLATFORM_SURPPORT_SECURE_OS), yes)

ifeq ($(SECURE_OS_OPTEE), yes)
    LOCAL_SHARED_LIBRARIES+= \
	    libteec

    LOCAL_C_INCLUDES += \
        $(TOP)/hardware/aw/optee_client-master/public
else
    LOCAL_SHARED_LIBRARIES+=\
        libtee_client
    LOCAL_C_INCLUDES += \
    	$(TOP)/hardware/aw/client-api
endif

endif #end# 'PLATFORM_SURPPORT_SECURE_OS == yes'

## compile libMemAdapter
LOCAL_MODULE:= libomx_MemAdapter
include $(BUILD_SHARED_LIBRARY)

endif
