
include $(top_srcdir)/Makefile.inc

pkglib_LTLIBRARIES = libcdc_vd_h264.la

libcdc_vd_h264_la_SOURCES = h264_hal.c \
							h264_mmco.c \
						   	h264_nalu.c \
							h264.c \
					   		h264_dec.c \
						   	h264_mvc.c

libcdc_vd_h264_la_CFLAGS = $(CFLAGS_CDCG)
LOCAL_INCLUDE = \
				-I$(top_srcdir)/include \
				-I$(top_srcdir)/vdecoder/include \
				-I$(top_srcdir)/ve/include \
				-I$(top_srcdir)/base/include \
				-I$(top_srcdir)/vdecoder


libcdc_vd_h264_la_CFLAGS += $(LOCAL_INCLUDE)


libcdc_vd_h264_la_LDFLAGS = $(LDFLAGS_CDCG)

libcdc_vd_h264_la_LIBADD = \
				$(top_srcdir)/vdecoder/libcdc_vdecoder.la \
                $(top_srcdir)/vdecoder/videoengine/libcdc_videoengine.la \
				$(top_srcdir)/ve/libcdc_ve.la

libcdc_vd_h264_la_LDFLAGS += $(LOCAL_LIB)

