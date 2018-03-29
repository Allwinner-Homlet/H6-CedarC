
include $(top_srcdir)/Makefile.inc

pkglib_LTLIBRARIES = libcdc_vd_h265.la

libcdc_vd_h265_la_SOURCES = h265_memory.c \
							h265.c \
							h265_debug.c \
						   	h265_ref.c \
						   	h265_drv.c \
							h265_md5.c \
						  	h265_parser.c \
						   	h265_register.c \
						   	h265_sha.c

libcdc_vd_h265_la_CFLAGS = $(CFLAGS_CDCG)
LOCAL_INCLUDE = \
				-I$(top_srcdir)/include \
				-I$(top_srcdir)/vdecoder/include \
				-I$(top_srcdir)/ve/include \
				-I$(top_srcdir)/base/include \
                                -I$(top_srcdir)/base/include/gralloc_metadata \
				-I$(top_srcdir)/vdecoder 


libcdc_vd_h265_la_CFLAGS += $(LOCAL_INCLUDE)


libcdc_vd_h265_la_LDFLAGS = $(LDFLAGS_CDCG)

libcdc_vd_h265_la_LIBADD = \
				$(top_srcdir)/vdecoder/libcdc_vdecoder.la \
				$(top_srcdir)/ve/libcdc_ve.la \
				$(top_srcdir)/vdecoder/videoengine/libcdc_videoengine.la

libcdc_vd_h265_la_LDFLAGS += $(LOCAL_LIB)

