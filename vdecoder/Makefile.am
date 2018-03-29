


include $(top_srcdir)/Makefile.inc

lib_LTLIBRARIES = libcdc_vdecoder.la

libcdc_vdecoder_la_SOURCES =    fbm.c \
						   pixel_format.c \
						   vdecoder.c \
						   sbm/sbmStream.c \
						   sbm/sbmFrameH264.c \
						   sbm/sbmFrameH265.c \
						   sbm/sbmFrameBase.c

libcdc_vdecoder_la_CFLAGS = $(CFLAGS_CDCG)
LOCAL_INCLUDE = -I./ \
                -I./videoengine \
                -I./include \
                -I$(top_srcdir)/include \
                -I$(top_srcdir)/base/include \
				-I$(top_srcdir)/base/include \
                -I$(top_srcdir)/ve/include

libcdc_vdecoder_la_CFLAGS += $(LOCAL_INCLUDE)


libcdc_vdecoder_la_LDFLAGS = $(LDFLAGS_CDCG)

libcdc_vdecoder_la_LIBADD = $(top_srcdir)/base/libcdc_base.la \
						$(top_srcdir)/ve/libcdc_ve.la \
						$(top_srcdir)/memory/libcdc_memory.la \
                        $(top_srcdir)/vdecoder/videoengine/libcdc_videoengine.la
LOCAL_LIB = -ldl -lpthread
libcdc_vdecoder_la_LDFLAGS += $(LOCAL_LIB)


